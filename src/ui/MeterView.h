#pragma once

// MeterView
// ---------
// Basic input/output peak + RMS metering with a delta indicator. Reads from
// MeterSnapshot (single-writer/single-reader) and applies its own ballistics
// so the audio thread doesn't carry the cost.

#include <juce_gui_basics/juce_gui_basics.h>
#include "../dsp/MeterTap.h"

namespace smex
{
	class MeterView : public juce::Component
	{
	public:
		MeterView();

		void setSnapshot (const MeterSnapshot& s) noexcept { latest_ = s; }
		void paint (juce::Graphics& g) override;

	private:
		MeterSnapshot latest_ {};

		// UI-side smoothed peak hold for stable visual reading.
		float displayInL_  { 0.0f };
		float displayInR_  { 0.0f };
		float displayOutL_ { 0.0f };
		float displayOutR_ { 0.0f };

		void drawBar (juce::Graphics& g, juce::Rectangle<int> r,
		              float linAmplitude, const juce::String& caption);
	};
}
