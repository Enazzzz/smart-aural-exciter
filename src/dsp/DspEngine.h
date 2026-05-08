#pragma once

// DspEngine
// ---------
// Orchestrates the V1 signal flow described in docs/spec/03-dsp-architecture.md.
// All audio-thread allocations happen in prepare(); process() must allocate nothing.

#include <juce_dsp/juce_dsp.h>

#include "../plugin/Parameters.h"
#include "ActivitySnapshot.h"
#include "Oversampler.h"
#include "AnalysisBank.h"
#include "HarmonicGenerator.h"
#include "HarshnessGuard.h"
#include "SafetyLimiter.h"
#include "MeterTap.h"

namespace smex
{
	class DspEngine
	{
	public:
		DspEngine();
		~DspEngine() = default;

		void prepare (double sampleRate, int maxBlockSize, int numChannels);
		void setQualityMode (QualityMode mode);

		int getLatencySamples() const noexcept { return reportedLatency_; }

		// Live parameter values, mirrored from APVTS. The processor pushes
		// these once per block so the engine doesn't walk the value tree on
		// the audio thread.
		struct LiveParams
		{
			float amount       { 0.35f };  // 0..1
			float tone         { 0.0f };   // -1..+1
			float sensitivity  { 0.5f };   // 0..1
			float mix          { 1.0f };   // 0..1
			float outputGain   { 1.0f };   // linear
			float guardAmount  { 0.6f };   // 0..1
			float headroomGain { 0.7079f };// linear, ~ -3 dB
		};

		void setParams (const LiveParams& p) noexcept { params_ = p; }

		void process (juce::AudioBuffer<float>& buffer, bool bypassedInternally);

		ActivityFrame  activitySnapshot() const noexcept { return activity_.snapshot(); }
		MeterSnapshot  meterSnapshot()    const noexcept { return meters_.snapshot(); }

	private:
		void runDcBlock (juce::AudioBuffer<float>& buffer) noexcept;
		void recomputeLatency() noexcept;

		double sampleRate_   { 0.0 };
		int    maxBlockSize_ { 0 };
		int    numChannels_  { 0 };

		QualityMode mode_ { QualityMode::Standard };
		int reportedLatency_ { 0 };

		LiveParams params_ {};

		// DC blocker state (one Y[n-1] / X[n-1] per channel).
		float dcXPrev_[2] { 0.0f, 0.0f };
		float dcYPrev_[2] { 0.0f, 0.0f };
		float dcCoeff_ { 0.0f };

		Oversampler        oversampler_;
		AnalysisBank       analysis_;
		HarmonicGenerator  harmonics_;
		HarshnessGuard     guard_;
		SafetyLimiter      safety_;
		MeterTap           meters_;
		ActivityPublisher  activity_;

		// Input snapshot taken at the start of process() so meters can compare
		// "what came in" against "what's going out" even though the chain
		// modifies buffers in place.
		juce::AudioBuffer<float> inputSnapshot_;
	};
}
