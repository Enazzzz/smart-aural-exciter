#pragma once

// SafetyLimiter
// -------------
// Final stage in the chain. Enforces the -1.0 dBFS / -1.0 dBTP ceiling
// from docs/spec/02-measurements.md section 5. Designed as a release-only
// soft-knee limiter (no lookahead) so we contribute zero added latency.
//
// Behavior at extreme overload: clamps gracefully, never emits NaN/Inf,
// recovers in tens of milliseconds when input drops back.

#include <juce_dsp/juce_dsp.h>

namespace smex
{
	class SafetyLimiter
	{
	public:
		SafetyLimiter();

		void prepare (double sampleRate, int maxBlockSize, int numChannels);
		void process (juce::AudioBuffer<float>& buffer);

		// Release-only design - zero added latency to PDC.
		int getLatencySamples() const noexcept { return 0; }

	private:
		// Hard ceiling target = -1.0 dBFS, knee starts ~1 dB earlier.
		static constexpr float kCeilingLin = 0.8912509f; // 10^(-1/20)
		static constexpr float kKneeStart  = 0.7943282f; // 10^(-2/20)

		double sampleRate_ { 0.0 };
		int    numChannels_ { 0 };

		// One gain-reduction state per channel; release behavior is shared.
		float gainState_[2] { 1.0f, 1.0f };
		float releaseCoef_ { 0.0f };
	};
}
