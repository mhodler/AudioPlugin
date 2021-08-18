/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope
{
	Slope_12,
	Slope_24,
	Slope_36,
	Slope_48
};

struct ChainSettings
{
	float peakFreq{ 0 }, peakGainInDecibels{ 0 }, peakQuality{ 1.f };
	float lowCutFreq{ 0 }, highCutFreq{ 0 };

	Slope lowCutSlope{ Slope::Slope_12 }, highCutSlope{ Slope::Slope_12 };
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);


//==============================================================================
/**
*/
class AudioPluginAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;



	static juce::AudioProcessorValueTreeState::ParameterLayout
		createParameterLayout();

	juce::AudioProcessorValueTreeState apvts { *this, nullptr, "Parameters", createParameterLayout()};
		
private:

	using Filter = juce::dsp::IIR::Filter<float>;

	using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

	using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

	MonoChain leftChain, rightChain;

	enum ChainPosition {
		LowCut,
		Peak,
		HighCut
	};

	void updatePeakFilter(const ChainSettings& chainSettings);
	using Coefficients = Filter::CoefficientsPtr;

	static void updateCoefficients(Coefficients& old, const Coefficients& replacements);

	template<int index, typename ChainType, typename CoefficientType>
	void update(ChainType& chain, const CoefficientType& coefficients)
	{
		updateCoefficients(chain.template get<index>().coefficients, coefficients[index]);
		chain.template setBypassed<index>(false);
	};
	
	template<typename ChainType, typename CoefficientType>
	void updateCutFilter(	ChainType& chain,
							const CoefficientType& cutCoefficients,
							const Slope& Slope)
	{
		chain.setBypassed<0>(true);
		chain.setBypassed<1>(true);
		chain.setBypassed<2>(true);
		chain.setBypassed<3>(true);

		switch (Slope) {

			case Slope_48:
			{
				update<3>(chain, cutCoefficients);
			}
		case Slope_36:
			{
				update<2>(chain, cutCoefficients);
			}

		case Slope_24:
			{
				update<1>(chain, cutCoefficients);
			}
		case Slope_12:
			{ 
				update<0>(chain, cutCoefficients);
			}
		
				/*case Slope_12: {
				*chain.get<0>().coefficients = *cutCoefficients[0];
				chain.setBypassed<0>(false);
				break;
			}


			case Slope_24: {
				*chain.get<0>().coefficients = *cutCoefficients[0];
				chain.setBypassed<0>(false);
				*chain.get<1>().coefficients = *cutCoefficients[1];
				chain.setBypassed<1>(false);
				break;
			}

			case Slope_36:
			{
				*chain.get<0>().coefficients = *cutCoefficients[0];
				chain.setBypassed<0>(false);
				*chain.get<1>().coefficients = *cutCoefficients[1];
				chain.setBypassed<1>(false);
				*chain.get<2>().coefficients = *cutCoefficients[2];
				chain.setBypassed<2>(false);
				break;
			}

			case Slope_48:
			{
				*chain.get<0>().coefficients = *cutCoefficients[0];
				chain.setBypassed<0>(false);
				*chain.get<1>().coefficients = *cutCoefficients[1];
				chain.setBypassed<1>(false);
				*chain.get<2>().coefficients = *cutCoefficients[2];
				chain.setBypassed<2>(false);
				*chain.get<3>().coefficients = *cutCoefficients[3];
				chain.setBypassed<3>(false);
				break;
			}*/
		}
	
	};


	void updateLowCutFilters(const ChainSettings& chainSettings);
	void updateHighCutFilters(const ChainSettings& chainSettings);
	void updateFilters();
	

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};