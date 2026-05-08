#pragma once

// HarmonicGenerator
// -----------------
// Selective excitation engine. Produces harmonic content from a soft asymmetric
// saturator and shapes it per-band in the spectral domain using the bandGain
// envelopes from AnalysisBank and the attenuation coefficients from
// HarshnessGuard. The shaped harmonics are then added on top of the original
// signal, leaving the dry path untouched in spectral magnitude.
//
// V1 implementation choice: per-block FFT, no overlap-add.
//   Pros:  zero added latency, deterministic per fixed block size, simple.
//   Cons:  minor cyclic-convolution artifacts at block boundaries on highly
//          transient material. Acceptable on master-bus content; revisit
//          post-V1 if validation flags audible block-edge clicks.
//
// (See docs/spec/03-dsp-architecture.md sections 3.4 / 7 for the open-decision
// guardrails this implementation operates inside.)

#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace smex
{
	class HarmonicGenerator
	{
	public:
		HarmonicGenerator();

		void prepare (double oversampledRate,
		              int maxOsBlock,
		              int numChannels,
		              const std::vector<float>& bandEdgesHz,
		              int numBands);

		// Modify wetBlock in place: wet[n] += filtered_harmonics(wet[n]).
		// `bandGain` and `attenuation` are length numBands. The effective
		// per-band excitation gain is bandGain[k] * attenuation[k] * amount.
		void process (juce::dsp::AudioBlock<float>& wetBlock,
		              const float* bandGain,
		              const float* attenuation,
		              int numBands,
		              float amount);

		// HarmonicGenerator currently reports zero added latency (per-block FFT).
		int getLatencySamples() const noexcept { return 0; }

	private:
		void ensureFftFor (int blockSize);

		double oversampledRate_ { 0.0 };
		int    numChannels_     { 0 };
		int    numBands_        { 0 };

		// FFT state. We adapt FFT size to the largest block size we've seen,
		// rounded up to the next power of 2.
		int fftOrder_ { 0 };
		int fftSize_  { 0 };
		std::unique_ptr<juce::dsp::FFT> fft_;

		// Per-channel scratch buffers. Allocated for the largest expected block.
		std::vector<float> harm_;
		std::vector<float> orig_;

		// Bin -> band index map for the current FFT size.
		std::vector<int> binToBand_;
		std::vector<float> bandEdgesHz_;
	};
}
