#pragma once

// MeterTap
// --------
// Publishes input level, output level, and their delta to the editor for
// the basic meter view (docs/spec/01-scope-freeze.md section 5.4).
//
// Single-writer (audio thread) / single-reader (UI thread) using atomic
// snapshot publishing - never blocks the audio thread.

#include <atomic>
#include <juce_dsp/juce_dsp.h>

namespace smex
{
	struct MeterSnapshot
	{
		float inPeakL  { 0.0f };
		float inPeakR  { 0.0f };
		float outPeakL { 0.0f };
		float outPeakR { 0.0f };
		float inRmsL   { 0.0f };
		float outRmsL  { 0.0f };
	};

	class MeterTap
	{
	public:
		MeterTap();

		void prepare (double sampleRate);

		// Capture peak/RMS over the buffer pair. Both buffers are at host rate
		// and aligned so the delta is meaningful.
		void captureInputAndOutput (const juce::AudioBuffer<float>& in,
		                            const juce::AudioBuffer<float>& out) noexcept;

		MeterSnapshot snapshot() const noexcept;

	private:
		double sampleRate_ { 0.0 };
		std::atomic<float> inPeakL_  { 0.0f };
		std::atomic<float> inPeakR_  { 0.0f };
		std::atomic<float> outPeakL_ { 0.0f };
		std::atomic<float> outPeakR_ { 0.0f };
		std::atomic<float> inRmsL_   { 0.0f };
		std::atomic<float> outRmsL_  { 0.0f };
	};
}
