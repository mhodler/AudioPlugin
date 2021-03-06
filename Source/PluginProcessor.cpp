/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"



//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
	juce::dsp::ProcessSpec spec;

	spec.maximumBlockSize = samplesPerBlock;

	spec.numChannels = 1;

	spec.sampleRate = sampleRate;

	leftChain.prepare(spec); //Channel output not just Mono
	rightChain.prepare(spec);

	updateFilters();
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

	updateFilters(); //Always update your parameters first

	juce::dsp::AudioBlock<float> block(buffer);
	auto leftBlock = block.getSingleChannelBlock(0); 
	auto rightBlock = block.getSingleChannelBlock(1);

	juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
	juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

	leftChain.process(leftContext);//sending buffer blocks to different channels
	rightChain.process(rightContext);

}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);

	//return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{	
	ChainSettings settings;

	settings.lowCutFreq= apvts.getRawParameterValue("LowCut Freq")->load();
	settings.highCutFreq= apvts.getRawParameterValue("HighCut Freq")->load();
	settings.peakFreq= apvts.getRawParameterValue("Peak Freq")->load();
	settings.peakGainInDecibels= apvts.getRawParameterValue("Peak Gain")->load();
	settings.peakQuality=apvts.getRawParameterValue("Peak Quality")->load();
	settings.lowCutSlope = static_cast<Slope>(static_cast<int>(apvts.getRawParameterValue("LowCut Slope")->load()));
	settings.highCutSlope = static_cast<Slope>(static_cast<int>(apvts.getRawParameterValue("HighCut Slope")->load()));
	
	//apvts.getParameter("LowCut Freq")->getValue();

	return settings;
}

void AudioPluginAudioProcessor::updatePeakFilter(const ChainSettings &chainSettings)
{
	auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
		chainSettings.peakFreq,
		chainSettings.peakQuality,
		juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));

	updateCoefficients(leftChain.get<ChainPosition::Peak>().coefficients, peakCoefficients);
	updateCoefficients(rightChain.get<ChainPosition::Peak>().coefficients, peakCoefficients);
}

void AudioPluginAudioProcessor::updateCoefficients(Coefficients& old, const Coefficients& replacements)
{
	*old = *replacements;
}




void AudioPluginAudioProcessor::updateLowCutFilters(const ChainSettings &chainSettings)
{
	auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
		getSampleRate(),
		2 * (chainSettings.lowCutSlope + 1));

	auto& leftLowCut = leftChain.get<ChainPosition::LowCut>();
	auto& rightLowCut = rightChain.get<ChainPosition::LowCut>();

	updateCutFilter(leftLowCut, cutCoefficients, chainSettings.lowCutSlope);
	updateCutFilter(rightLowCut, cutCoefficients, chainSettings.lowCutSlope);
	
	
}



void AudioPluginAudioProcessor::updateHighCutFilters(const ChainSettings &chainSettings)
{
	auto highCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq,
		getSampleRate(),
		2 * (chainSettings.highCutSlope + 1));
	
	auto& rightHighCut = rightChain.get<ChainPosition::HighCut>();
	auto& leftHighCut = leftChain.get<ChainPosition::HighCut>();

	updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
	updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);

}


void AudioPluginAudioProcessor::updateFilters()
{
	auto chainSettings = getChainSettings(apvts);

	updateLowCutFilters(chainSettings);
	updateHighCutFilters(chainSettings);
	updatePeakFilter(chainSettings);
}

juce::AudioProcessorValueTreeState::ParameterLayout
	AudioPluginAudioProcessor::createParameterLayout()
	{
	juce::AudioProcessorValueTreeState::ParameterLayout layout;

	layout.add(std::make_unique<juce::AudioParameterFloat>(	"LowCut Freq",
															"Low Cutt Frequency",
															juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),	//min value, max value, stepsize, slider distribution
															20.f));														//starting value


	layout.add(std::make_unique<juce::AudioParameterFloat>(	"HighCut Freq",
															"High Cut Frequency",
															juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
															20000.f));

	layout.add(std::make_unique<juce::AudioParameterFloat>(	"Peak Freq",
															"Peak Frequency",
															juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
															750.f));


	layout.add(std::make_unique<juce::AudioParameterFloat>(	"Peak Gain",
															"Peak Gain Frequency",
															juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
															0.0f));

	layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Quality",
															"Peak Quality",
														    juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
															1.f));

	juce::StringArray stringArray;
	for (int i = 0;i < 4; ++i) {
		juce::String str;
		str << (12 + i*12);
		str << " db/Oct ";
		stringArray.add(str);
	}

	layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope","LowCut Slope", stringArray, 0));
	layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", stringArray, 0));
	

	
	return layout;
	}



//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
