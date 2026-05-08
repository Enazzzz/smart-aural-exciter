#include "AdvancedPanel.h"

#include "../plugin/Parameters.h"

namespace smex
{
	AdvancedPanel::AdvancedPanel (juce::AudioProcessorValueTreeState& apvts)
	{
		auto setupCombo = [this](ComboBox& c, juce::Label& l, const juce::String& text)
		{
			addAndMakeVisible (c);
			l.setText (text, juce::dontSendNotification);
			l.setFont (juce::Font (12.0f));
			addAndMakeVisible (l);
		};

		auto setupSlider = [this](Slider& s, juce::Label& l, const juce::String& text)
		{
			s.setSliderStyle (juce::Slider::LinearHorizontal);
			s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 16);
			addAndMakeVisible (s);
			l.setText (text, juce::dontSendNotification);
			l.setFont (juce::Font (12.0f));
			addAndMakeVisible (l);
		};

		// Quality combo: must match the StringArray order in createParameterLayout.
		quality_.addItem ("Standard", 1);
		quality_.addItem ("High",     2);
		setupCombo (quality_, qualityLabel_, "Quality");

		setupSlider (guard_,   guardLabel_,   "Guard");

		activityScale_.addItem ("Linear", 1);
		activityScale_.addItem ("Log",    2);
		activityScale_.addItem ("Auto",   3);
		setupCombo (activityScale_, activityLabel_, "Activity Scale");

		setupSlider (headroom_, headroomLabel_, "Headroom (dB)");

		qualityAtt_  = std::make_unique<ComboAtt>  (apvts, ParamId::qualityMode,   quality_);
		guardAtt_    = std::make_unique<SliderAtt> (apvts, ParamId::guardAmount,   guard_);
		activityAtt_ = std::make_unique<ComboAtt>  (apvts, ParamId::activityScale, activityScale_);
		headroomAtt_ = std::make_unique<SliderAtt> (apvts, ParamId::headroomTrim,  headroom_);
	}

	void AdvancedPanel::resized()
	{
		auto r = getLocalBounds().reduced (4);
		auto rowH = r.getHeight() / 2;

		auto row1 = r.removeFromTop (rowH);
		auto col1 = row1.removeFromLeft (row1.getWidth() / 2);
		qualityLabel_.setBounds (col1.removeFromLeft (90));
		quality_.setBounds      (col1);
		activityLabel_.setBounds (row1.removeFromLeft (110));
		activityScale_.setBounds (row1);

		auto row2 = r;
		auto col2 = row2.removeFromLeft (row2.getWidth() / 2);
		guardLabel_.setBounds (col2.removeFromLeft (90));
		guard_.setBounds      (col2);
		headroomLabel_.setBounds (row2.removeFromLeft (110));
		headroom_.setBounds      (row2);
	}

	void AdvancedPanel::paint (juce::Graphics& g)
	{
		g.setColour (juce::Colours::white.withAlpha (0.04f));
		g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
	}
}
