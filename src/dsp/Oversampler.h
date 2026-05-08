#pragma once

// Oversampler
// -----------
// Standard quality = 2x, High quality = 4x. Wraps juce::dsp::Oversampling so
// the rest of the chain doesn't have to worry about the up/down details.
//
// Design contract
// ---------------
// processUp() returns a WRITABLE AudioBlock pointing at the JUCE oversampler's
// internal up-rate buffer. The engine modifies that block in place. processDown()
// then downsamples whatever is in the internal block back to host rate. This
// avoids any extra copies in the hot path and keeps the IIR filter state
// consistent between up/down halves.
//
// In parallel, processUp() also captures a private dry copy at the up rate
// for delay-aligned dry/wet mixing later in the chain.

#include <juce_dsp/juce_dsp.h>
#include "../plugin/Parameters.h"

namespace smex
{
	class Oversampler
	{
	public:
		Oversampler();

		void prepare (double hostSampleRate, int maxBlockSize, int numChannels);
		void setMode (QualityMode mode);

		// Latency contributed by the oversampler pair, reported in HOST samples.
		int getLatencySamples() const noexcept;

		double getOversampledRate() const noexcept;
		int    getMaxOversampledBlock() const noexcept;

		// Run the up-sampler. Returns a writable view onto the JUCE-owned
		// up-rate buffer. The dry copy is captured into a private buffer
		// accessible via getDryBlock().
		juce::dsp::AudioBlock<float> processUp (const juce::AudioBuffer<float>& host);

		// Captured (already up-rate) dry signal. Returned as a writable block
		// for JUCE API simplicity; callers must treat it as read-only.
		juce::dsp::AudioBlock<float> getDryBlock() noexcept;

		// Down-sample whatever is currently in the JUCE internal block back into
		// `host` (in place).
		void processDown (juce::AudioBuffer<float>& host);

		// Internal bypass keeps PDC stable while skipping the wet DSP work.
		void processBypassed (juce::AudioBuffer<float>& host);

	private:
		std::unique_ptr<juce::dsp::Oversampling<float>> osStandard_;
		std::unique_ptr<juce::dsp::Oversampling<float>> osHigh_;

		QualityMode mode_ { QualityMode::Standard };
		double hostSampleRate_ { 0.0 };
		int    maxBlockSize_   { 0 };
		int    numChannels_    { 0 };

		// The dry tap lives in our buffer at oversampled rate, sized to the
		// largest possible up block. Captured during processUp().
		juce::AudioBuffer<float> dryTap_;
		int dryTapValidSamples_ { 0 };

		juce::dsp::Oversampling<float>* current() noexcept
		{
			return mode_ == QualityMode::High ? osHigh_.get() : osStandard_.get();
		}
		const juce::dsp::Oversampling<float>* current() const noexcept
		{
			return mode_ == QualityMode::High ? osHigh_.get() : osStandard_.get();
		}
	};
}
