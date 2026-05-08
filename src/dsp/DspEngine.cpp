#include "DspEngine.h"

#include <cmath>

namespace smex
{
	DspEngine::DspEngine() = default;

	void DspEngine::prepare (double sampleRate, int maxBlockSize, int numChannels)
	{
		sampleRate_   = sampleRate;
		maxBlockSize_ = maxBlockSize;
		numChannels_  = juce::jlimit (1, 2, numChannels);

		// Standard transposed-direct-form 1st-order high-pass at ~5 Hz:
		// y[n] = a * (y[n-1] + x[n] - x[n-1])
		// where a = exp(-2*pi*fc/fs). Stable, near-zero CPU, no resonance.
		dcCoeff_ = std::exp (-2.0f * juce::MathConstants<float>::pi * 5.0f
			/ static_cast<float>(sampleRate_));
		dcXPrev_[0] = dcXPrev_[1] = 0.0f;
		dcYPrev_[0] = dcYPrev_[1] = 0.0f;

		oversampler_.prepare (sampleRate_, maxBlockSize_, numChannels_);

		const auto osRate  = oversampler_.getOversampledRate();
		const auto osBlock = oversampler_.getMaxOversampledBlock();

		analysis_.prepare  (osRate, osBlock, numChannels_);
		harmonics_.prepare (osRate, osBlock, numChannels_,
		                    analysis_.getBandEdgesHz(),
		                    analysis_.getNumBands());
		guard_.prepare     (analysis_.getNumBands());
		safety_.prepare    (sampleRate_, maxBlockSize_, numChannels_);
		meters_.prepare    (sampleRate_);

		inputSnapshot_.setSize (numChannels_, maxBlockSize_, false, true, true);

		recomputeLatency();
	}

	void DspEngine::setQualityMode (QualityMode mode)
	{
		if (mode == mode_) return;
		mode_ = mode;
		oversampler_.setMode (mode_);
		analysis_.setQualityMode (mode_);
		recomputeLatency();
	}

	void DspEngine::recomputeLatency() noexcept
	{
		const auto osLat       = oversampler_.getLatencySamples();
		const auto analysisLat = analysis_.getLatencySamples();
		const auto safetyLat   = safety_.getLatencySamples();
		reportedLatency_ = osLat + analysisLat + safetyLat;
	}

	void DspEngine::runDcBlock (juce::AudioBuffer<float>& buffer) noexcept
	{
		const auto numCh = juce::jmin (numChannels_, buffer.getNumChannels());
		const auto numSamples = buffer.getNumSamples();

		for (int ch = 0; ch < numCh; ++ch)
		{
			auto* d = buffer.getWritePointer (ch);
			float xPrev = dcXPrev_[ch];
			float yPrev = dcYPrev_[ch];

			for (int i = 0; i < numSamples; ++i)
			{
				const float x = d[i];
				const float y = dcCoeff_ * (yPrev + x - xPrev);
				d[i] = y;
				xPrev = x;
				yPrev = y;
			}

			dcXPrev_[ch] = xPrev;
			dcYPrev_[ch] = yPrev;
		}
	}

	void DspEngine::process (juce::AudioBuffer<float>& buffer, bool bypassedInternally)
	{
		const juce::ScopedNoDenormals noDenormals;

		// Snapshot input BEFORE we modify the buffer in place. Used by the
		// meter for the "input vs output" delta and by the dry tap if needed.
		const auto numCh = juce::jmin (numChannels_, buffer.getNumChannels());
		const auto N = buffer.getNumSamples();
		for (int ch = 0; ch < numCh; ++ch)
			juce::FloatVectorOperations::copy (inputSnapshot_.getWritePointer (ch),
			                                    buffer.getReadPointer (ch), N);

		runDcBlock (buffer);

		if (bypassedInternally)
		{
			oversampler_.processBypassed (buffer);
			meters_.captureInputAndOutput (inputSnapshot_, buffer);
			return;
		}

		// 1) Up-sample. wetBlock points at the JUCE-owned up-rate buffer that
		//    the wet path mutates in place. dryBlock is our delay-aligned dry copy.
		auto wetBlock = oversampler_.processUp (buffer);
		auto dryBlock = oversampler_.getDryBlock();

		// 2) Analyse the OS signal -> bandGain[N], harshnessMask[N].
		analysis_.process (wetBlock, params_.tone, params_.sensitivity);

		auto& frame = activity_.writable();
		analysis_.fillActivityFrame (frame);

		// 3) Compute harshness guard attenuation (no audio work yet - the
		//    actual attenuation rides into the harmonic generator's FFT pass
		//    so we don't pay for a second FFT/IFFT).
		guard_.process (analysis_.getHarshnessMask(),
		                analysis_.getNumBands(),
		                params_.guardAmount);

		// 4) Harmonics: generate selectively per band, applying the guard
		//    attenuation in the same spectral pass.
		harmonics_.process (wetBlock,
		                    analysis_.getBandGains(),
		                    guard_.getAttenuation(),
		                    analysis_.getNumBands(),
		                    params_.amount);

		// 5) Dry/wet crossfade at OS rate. Trims come AFTER the mix per spec.
		const auto wetN = static_cast<int>(wetBlock.getNumSamples());
		for (int ch = 0; ch < numCh; ++ch)
		{
			auto* w = wetBlock.getChannelPointer (static_cast<size_t>(ch));
			const auto* dry = dryBlock.getChannelPointer (static_cast<size_t>(ch));
			const float wetGain = params_.mix;
			const float dryGain = 1.0f - params_.mix;
			for (int i = 0; i < wetN; ++i)
				w[i] = w[i] * wetGain + dry[i] * dryGain;
		}

		// 6) Headroom + output trims after the mix, before downsample (spec
		//    docs/spec/03-dsp-architecture.md sections 3.7 and 3.8).
		const float trim = params_.headroomGain * params_.outputGain;
		wetBlock.multiplyBy (trim);

		// 7) Down-sample back to host rate, writing into `buffer`.
		oversampler_.processDown (buffer);

		// 8) Final safety stage at host rate.
		safety_.process (buffer);

		// 9) Publish snapshots for the UI.
		meters_.captureInputAndOutput (inputSnapshot_, buffer);
		activity_.publish();
	}
}
