#include "SimplePanel.h"

#include "../plugin/Parameters.h"

namespace smex
{
	void SimplePanel::initKnob (Knob& k, const juce::String& display)
	{
		k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
		k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
		addAndMakeVisible (k.slider);

		k.label.setText (display, juce::dontSendNotification);
		k.label.setJustificationType (juce::Justification::centred);
		k.label.setFont (juce::Font (12.0f));
		addAndMakeVisible (k.label);
	}

	SimplePanel::SimplePanel (juce::AudioProcessorValueTreeState& apvts)
	{
		initKnob (amount_,      "Amount");
		initKnob (tone_,        "Tone");
		initKnob (sensitivity_, "Sensitivity");
		initKnob (mix_,         "Mix");
		initKnob (output_,      "Output");

		amount_.att      = std::make_unique<SliderAtt> (apvts, ParamId::amount,      amount_.slider);
		tone_.att        = std::make_unique<SliderAtt> (apvts, ParamId::tone,        tone_.slider);
		sensitivity_.att = std::make_unique<SliderAtt> (apvts, ParamId::sensitivity, sensitivity_.slider);
		mix_.att         = std::make_unique<SliderAtt> (apvts, ParamId::mix,         mix_.slider);
		output_.att      = std::make_unique<SliderAtt> (apvts, ParamId::output,      output_.slider);
	}

	void SimplePanel::layoutKnob (Knob& k, juce::Rectangle<int> r)
	{
		auto label = r.removeFromBottom (16);
		k.label.setBounds (label);
		k.slider.setBounds (r);
	}

	void SimplePanel::resized()
	{
		auto r = getLocalBounds().reduced (4);
		const int knobW = r.getWidth() / 5;

		layoutKnob (amount_,      r.removeFromLeft (knobW));
		layoutKnob (tone_,        r.removeFromLeft (knobW));
		layoutKnob (sensitivity_, r.removeFromLeft (knobW));
		layoutKnob (mix_,         r.removeFromLeft (knobW));
		layoutKnob (output_,      r);
	}

	void SimplePanel::paint (juce::Graphics& g)
	{
		g.setColour (juce::Colours::white.withAlpha (0.05f));
		g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
	}
}
