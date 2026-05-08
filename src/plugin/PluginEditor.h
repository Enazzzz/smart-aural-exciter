#pragma once

// PluginEditor
// ------------
// Hosts the simple panel (5 controls), the optional advanced panel, the meter
// view, and the activity view. Layout is straightforward: simple panel and
// meters always visible, advanced panel collapsed behind a disclosure toggle.

#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"
#include "../ui/SimplePanel.h"
#include "../ui/AdvancedPanel.h"
#include "../ui/MeterView.h"
#include "../ui/ActivityView.h"

namespace smex
{
	class PluginEditor : public juce::AudioProcessorEditor,
	                     private juce::Timer
	{
	public:
		explicit PluginEditor (PluginProcessor& p);
		~PluginEditor() override = default;

		void paint (juce::Graphics& g) override;
		void resized() override;

	private:
		void timerCallback() override;

		PluginProcessor& processor_;

		SimplePanel    simple_;
		AdvancedPanel  advanced_;
		MeterView      meters_;
		ActivityView   activity_;

		juce::ToggleButton advancedToggle_ { "Advanced" };

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
	};
}
