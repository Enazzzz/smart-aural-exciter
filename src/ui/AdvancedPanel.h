#pragma once

// AdvancedPanel
// -------------
// Hidden by default behind the "Advanced" disclosure toggle in the editor.
// Holds the four advanced controls from docs/spec/01-scope-freeze.md
// section 5.3 (quality mode, guard amount, activity-view scaling,
// internal headroom trim).

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace smex
{
	class AdvancedPanel : public juce::Component
	{
	public:
		explicit AdvancedPanel (juce::AudioProcessorValueTreeState& apvts);

		void resized() override;
		void paint (juce::Graphics& g) override;

	private:
		using Slider     = juce::Slider;
		using ComboBox   = juce::ComboBox;
		using SliderAtt  = juce::AudioProcessorValueTreeState::SliderAttachment;
		using ComboAtt   = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

		ComboBox quality_;
		Slider   guard_;
		ComboBox activityScale_;
		Slider   headroom_;

		juce::Label qualityLabel_, guardLabel_, activityLabel_, headroomLabel_;

		std::unique_ptr<ComboAtt>  qualityAtt_;
		std::unique_ptr<SliderAtt> guardAtt_;
		std::unique_ptr<ComboAtt>  activityAtt_;
		std::unique_ptr<SliderAtt> headroomAtt_;
	};
}
