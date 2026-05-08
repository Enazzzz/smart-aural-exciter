#pragma once

// SimplePanel
// -----------
// The five always-visible top-level controls from
// docs/spec/01-scope-freeze.md section 5.2.

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace smex
{
	class SimplePanel : public juce::Component
	{
	public:
		explicit SimplePanel (juce::AudioProcessorValueTreeState& apvts);

		void resized() override;
		void paint (juce::Graphics& g) override;

	private:
		using Slider     = juce::Slider;
		using SliderAtt  = juce::AudioProcessorValueTreeState::SliderAttachment;
		using Attachment = std::unique_ptr<SliderAtt>;

		// Compact knob+label pair.
		struct Knob
		{
			Slider     slider;
			juce::Label label;
			Attachment att;
		};

		Knob amount_, tone_, sensitivity_, mix_, output_;

		void layoutKnob (Knob& k, juce::Rectangle<int> r);
		void initKnob   (Knob& k, const juce::String& display);
	};
}
