#include "HarmonicGenerator.h"

#include "ErbBands.h"

#include <cmath>

namespace smex
{
	HarmonicGenerator::HarmonicGenerator() = default;

	static int nextPow2 (int v)
	{
		int p = 1;
		while (p < v) p <<= 1;
		return p;
	}

	static int log2i (int v)
	{
		int order = 0;
		while ((1 << order) < v) ++order;
		return order;
	}

	void HarmonicGenerator::prepare (double oversampledRate,
	                                  int maxOsBlock,
	                                  int numChannels,
	                                  const std::vector<float>& bandEdgesHz,
	                                  int numBands)
	{
		oversampledRate_ = oversampledRate;
		numChannels_     = juce::jlimit (1, 2, numChannels);
		numBands_        = numBands;
		bandEdgesHz_     = bandEdgesHz;

		ensureFftFor (maxOsBlock);
	}

	void HarmonicGenerator::ensureFftFor (int blockSize)
	{
		const int n = nextPow2 (juce::jmax (blockSize, 32));
		if (n == fftSize_) return;

		fftSize_  = n;
		fftOrder_ = log2i (fftSize_);
		fft_      = std::make_unique<juce::dsp::FFT> (fftOrder_);

		// performRealOnlyForwardTransform expects a buffer of size 2*N
		// (real input expanded into complex output).
		harm_.assign (static_cast<size_t>(fftSize_ * 2), 0.0f);
		orig_.assign (static_cast<size_t>(fftSize_ * 2), 0.0f);

		// Rebuild bin->band map. The ErbLayout passed in via bandEdgesHz_
		// uses one extra edge entry; reconstruct an ErbLayout-compatible
		// proxy so we can call makeBinToBandMap.
		ErbLayout proxy;
		proxy.edgeHz   = bandEdgesHz_;
		proxy.numBands = numBands_;
		proxy.centerHz.assign (static_cast<size_t>(numBands_), 0.0f);
		binToBand_ = makeBinToBandMap (proxy, fftSize_, oversampledRate_);
	}

	void HarmonicGenerator::process (juce::dsp::AudioBlock<float>& wetBlock,
	                                  const float* bandGain,
	                                  const float* attenuation,
	                                  int numBands,
	                                  float amount)
	{
		const auto numCh = juce::jmin (numChannels_, static_cast<int>(wetBlock.getNumChannels()));
		const auto N     = static_cast<int>(wetBlock.getNumSamples());
		jassert (numBands == numBands_);

		ensureFftFor (N);

		const int numBins = fftSize_ / 2;
		const float invFftSize = 1.0f / static_cast<float>(fftSize_);

		for (int ch = 0; ch < numCh; ++ch)
		{
			auto* w = wetBlock.getChannelPointer (static_cast<size_t>(ch));

			// Build harmonic-only signal: harm[n] = soft_sat(x) - x.
			// The 2x pre-gain pushes the saturator into a usable range; the
			// post-divide by 2 normalises so that a unit-amplitude sine input
			// produces predictable harmonic levels.
			for (int n = 0; n < N; ++n)
			{
				const float x = w[n];
				const float sat = std::tanh (2.0f * x) * 0.5f;
				harm_[static_cast<size_t>(n)] = sat - x;
			}
			for (int n = N; n < fftSize_; ++n)
				harm_[static_cast<size_t>(n)] = 0.0f;

			// Forward FFT (real-only). JUCE packs the result as interleaved
			// complex pairs in the same buffer.
			fft_->performRealOnlyForwardTransform (harm_.data());

			// Per-bin spectral modulation: multiply by per-band gain envelope
			// from the analysis bank, scaled by the user's Amount.
			//
			// JUCE's RealOnly format places bin k at indices [2*k, 2*k+1]
			// for k in [0, N/2]. We use the band index of bin k for both halves.
			for (int bin = 0; bin <= numBins; ++bin)
			{
				const int band = (bin < static_cast<int>(binToBand_.size()))
					? binToBand_[static_cast<size_t>(bin)] : 0;
				const float bg = (band >= 0 && band < numBands)
					? bandGain[band] : 0.0f;
				const float at = (band >= 0 && band < numBands)
					? attenuation[band] : 1.0f;
				const float coef = bg * at * amount;

				const auto idxRe = static_cast<size_t>(2 * bin);
				const auto idxIm = idxRe + 1;
				if (idxIm < harm_.size())
				{
					harm_[idxRe] *= coef;
					harm_[idxIm] *= coef;
				}
			}

			// Inverse FFT, then add the shaped harmonic content to the
			// original wet signal. The 1/N factor undoes the FFT scaling.
			fft_->performRealOnlyInverseTransform (harm_.data());

			for (int n = 0; n < N; ++n)
				w[n] += harm_[static_cast<size_t>(n)] * invFftSize;
		}
	}
}
