/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
//Implementing costumized Look And Feel for the GUI
void LookAndFeel::drawRotarySlider(juce::Graphics& g,
													int x,
													int y,
													int width,
													int height,
													float sliderPosProportional,
													float rotaryStartAngle,
													float rotaryEndAngle, juce::Slider& slider)
{
	using namespace juce;
	auto bounds = Rectangle<float>(x, y, width, height);

	g.setColour(Colour(64u, 59u, 62u));
	g.fillEllipse(bounds);

	g.setColour(Colour(255u, 154, 1u));
	g.drawEllipse(bounds, 1.f);


	if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
	{
		auto center = bounds.getCentre();

		Path p;

		Rectangle<float> r;
		r.setLeft(center.getX() - 2);
		r.setRight(center.getX() + 2);
		r.setTop(bounds.getY());
		r.setBottom(center.getY());

		p.addRoundedRectangle(r, 2.f);

		jassert(rotaryStartAngle < rotaryEndAngle);

		auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

		p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

		g.fillPath(p);

		g.setFont(rswl->getTextHeight());
		auto text = rswl->getDisplayString();
		auto strWidth = g.getCurrentFont().getStringWidth(text);

		r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
		r.setCentre(bounds.getCentre());

		g.setColour(Colour(63u, 60u,84u));
		g.fillRect(r);

		g.setColour(Colours::green);
		g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);


	}



	
}

//==============================================================================

void RotarySliderWithLabels::paint(juce::Graphics &g)
{
	using namespace juce;

	auto startAng = degreesToRadians(180.f + 45.f);
	auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

	auto range = getRange();

	auto sliderBounds = getSliderBounds();

	/*g.setColour(Colours::red);
	g.drawRect(getLocalBounds());
	g.setColour(Colours::yellow);
	g.drawRect(sliderBounds);*/

	getLookAndFeel().drawRotarySlider(	g,
										sliderBounds.getX(),
										sliderBounds.getY(),
										sliderBounds.getWidth(),
										sliderBounds.getHeight(),
										jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
										startAng,
										endAng,
										*this);

	auto center = sliderBounds.toFloat().getCentre();
	auto radius = sliderBounds.getWidth() * 0.5f;

	g.setColour(Colour(0u, 172u, 1u));
	g.setFont(getTextHeight());

	auto numChoices =labels.size();
	for (int i = 0; i < numChoices; ++i) {
		auto pos = labels[i].pos;
		jassert(0.f <= pos);
		jassert(pos <= 1.f);

		auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);

		auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang); // a little bit past the Circle

			Rectangle<float> r;
		auto str = labels[i].label;
		r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
		r.setCentre(c);
		r.setY(r.getY() + getTextHeight()); // shifting labels down 
		g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
	}



}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
	auto bounds = getLocalBounds();

	auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

	size -= getTextHeight() * 2;
	juce::Rectangle<int> r;
	r.setSize(size, size);
	r.setCentre(bounds.getCentreX(), 0);
	r.setY(2);

	return r;
	//return getLocalBounds();
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
	//return juce::String(getValue());

	if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
		return choiceParam->getCurrentChoiceName();

	juce::String str;
	bool addK = false;

	if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
	{
		float val = getValue();

		if (val > 999.f) {
			val /= 1000.f;
			addK = true;
		}

		str = juce::String(val, (addK ? 2: 0));
	}
	else
	{
		jassertfalse;
	}

	if (suffix.isNotEmpty())
	{
		str << " ";
		if (addK)
			str << "k";

		str << suffix;
	}
	
	return str;
}

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p),
	peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
	peakGainSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "dB"),
	peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Freq"), ""),
	lowCutSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
	highCutSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
	lowCutSlopeSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "dB/Oct"),
	highCutSlopeSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "dB/Oct"),

	peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
	highCutSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutSlider),
	lowCutSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutSlider),
	peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
	peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
	lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
	highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider)


{


    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

	peakFreqSlider.labels.add({ 0.f, "20Hz" });
	peakFreqSlider.labels.add({ 1.f, "20kHz" });

	peakGainSlider.labels.add({ 0.f, "-24dB" });
	peakGainSlider.labels.add({ 1.f, "+24dB" });

	peakQualitySlider.labels.add({ 0.f, "0.1" });
	peakQualitySlider.labels.add({ 1.f, "10.0" });

	lowCutSlider.labels.add({ 0.f, "20Hz" });
	lowCutSlider.labels.add({ 1.f, "20kHz" });

	highCutSlider.labels.add({ 0.f, "20Hz" });
	highCutSlider.labels.add({ 1.f, "20kHz" });

	lowCutSlopeSlider.labels.add({ 0.0f, "12" });
	lowCutSlopeSlider.labels.add({ 1.f, "48" });

	highCutSlopeSlider.labels.add({ 0.0f, "12" });
	highCutSlopeSlider.labels.add({ 1.f, "48" });

	for (auto* comp : getComps() ) 
	{
		addAndMakeVisible(comp);
	}



    setSize (600, 400);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void AudioPluginAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
	auto bounds = getLocalBounds();
	//auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);

	auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
	auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);

	lowCutSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
	lowCutSlopeSlider.setBounds(lowCutArea);
	highCutSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
	highCutSlopeSlider.setBounds(highCutArea);

	peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
	peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
	peakQualitySlider.setBounds(bounds);


}

std::vector<juce::Component*> AudioPluginAudioProcessorEditor:: getComps()
{
	return
	{
		&peakFreqSlider,
		&peakGainSlider,
		&peakQualitySlider,
		&lowCutSlider,
		&highCutSlider,
		&highCutSlopeSlider,
		&lowCutSlopeSlider
	};
}
