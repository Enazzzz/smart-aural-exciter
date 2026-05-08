#pragma once

// AnalysisBank
// ------------
// Perceptual analysis stage. Consumes the oversampled wet/dry signal (read-only)
// and produces two control vectors used by HarmonicGenerator and HarshnessGuard:
//
//   bandGain[N]      - 0..1 per-band excitation gain envelope
//   harshnessMask[N] - 0..1 per-band attenuation factor for the guard
//
// The bank is STFT-based so we can share the same analysis window with the
// per-band gain modulation in HarmonicGenerator (see docs/spec/03-dsp-architecture.md
// section 7's note about polyphase FFT being an allowed implementation choice).
//
// The bank consumes audio one block at a time and runs as many analysis hops
// per block as fit. Per-band envelopes use one-pole attack/release smoothing
// to avoid audible pumping (defaults: 5 ms attack, 80 ms release).

#include <juce_dsp/juce_dsp.h>
#include <vector>

#include "../plugin/Parameters.h"
#include "ActivitySnapshot.h"
#include "ErbBands.h"

namespace smex
{
	class AnalysisBank
	{
	public:
		AnalysisBank();

		void prepare (double oversampledRate, int maxOsBlock, int numChannels);
		void setQualityMode (QualityMode mode);

		// Analyse the up-rate block. The caller passes the OS-rate AudioBlock
		// (read-only). Outputs go into the bank's internal arrays accessible
		// via the getters below.
		void process (const juce::dsp::AudioBlock<float>& osBlock,
		              float toneNorm,
		              float sensitivityNorm);

		const float* getBandGains()      const noexcept { return bandGain_.data(); }
		const float* getHarshnessMask() const noexcept { return harshnessMask_.data(); }
		int          getNumBands()       const noexcept { return numBands_; }

		const std::vector<float>& getBandEdgesHz() const noexcept { return layout_.edgeHz; }

		// Latency contributed by analysis = analysis hop alignment delay.
		// For an STFT of size W with hop H, the inherent algorithmic delay
		// is one window for the lookahead the wet path needs to align with
		// the analysed control envelopes.
		int getLatencySamples() const noexcept { return latencySamples_; }

		// Snapshot the current envelopes into the activity frame for the UI.
		void fillActivityFrame (ActivityFrame& frame) const noexcept;

	private:
		void setupForRate (double oversampledRate, int numChannels);
		void runOneHop() noexcept;

		double oversampledRate_ { 0.0 };
		int    numChannels_     { 0 };
		QualityMode mode_       { QualityMode::Standard };

		// FFT config. fftSize is set in prepare based on quality mode and
		// sample rate; window length matches fftSize, hop = fftSize / 2.
		int fftOrder_  { 10 }; // 1024
		int fftSize_   { 1024 };
		int hopSize_   { 512 };
		int latencySamples_ { 0 };

		std::unique_ptr<juce::dsp::FFT> fft_;

		// Hann window applied to each analysis frame.
		std::vector<float> window_;

		// Ring buffer per channel for accumulating samples into hop-sized
		// chunks. We mix to mono inside the bank because analysis is shared
		// across channels.
		std::vector<float> ring_;
		int ringWrite_ { 0 };
		int samplesSinceLastHop_ { 0 };

		// Scratch buffers for the FFT.
		std::vector<float> frame_;
		std::vector<float> magnitudes_;

		// ERB band layout and bin-to-band mapping precomputed at prepare time.
		ErbLayout layout_;
		std::vector<int> binToBand_;
		int numBands_ { 24 };

		// Per-band state.
		std::vector<float> bandEnergyDb_;     // raw per-band magnitude in dB
		std::vector<float> bandGain_;          // 0..1 smoothed
		std::vector<float> harshnessMask_;     // 0..1 smoothed
		std::vector<float> bandFlatness_;      // tonal vs noisy estimate

		// Hop scratch (kept as members so runOneHop never allocates).
		std::vector<float> bandPow_;
		std::vector<int>   bandCount_;

		// Smoothing coefficients per direction.
		float attackCoef_  { 0.0f };
		float releaseCoef_ { 0.0f };

		// Cached params from the most recent process() call.
		float toneNorm_        { 0.0f };
		float sensitivityNorm_ { 0.5f };
	};
}
