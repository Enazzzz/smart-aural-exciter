#include "AnalysisBank.h"

#include <algorithm>
#include <cmath>

namespace smex
{
	AnalysisBank::AnalysisBank() = default;

	void AnalysisBank::prepare (double oversampledRate, int maxOsBlock, int numChannels)
	{
		juce::ignoreUnused (maxOsBlock);
		setupForRate (oversampledRate, numChannels);
	}

	void AnalysisBank::setQualityMode (QualityMode mode)
	{
		mode_ = mode;
		// FFT size could change between modes if we wanted finer resolution
		// in High mode. For V1 we keep FFT size constant and let the
		// oversampler do the heavy lifting; analysis stays at 1024 samples
		// at the OS rate, which gives ~10 ms of smoothing window at 96 kHz OS.
	}

	void AnalysisBank::setupForRate (double oversampledRate, int numChannels)
	{
		oversampledRate_ = oversampledRate;
		numChannels_     = juce::jlimit (1, 2, numChannels);

		// 1024-point FFT at OS rate is a good compromise between time/freq
		// resolution and CPU. The Hann window's effective bandwidth lands
		// roughly at one ERB at 1 kHz which matches our band layout.
		fftOrder_ = 10;
		fftSize_  = 1 << fftOrder_;
		hopSize_  = fftSize_ / 2;
		fft_      = std::make_unique<juce::dsp::FFT> (fftOrder_);

		window_.resize (static_cast<size_t>(fftSize_));
		juce::dsp::WindowingFunction<float>::fillWindowingTables (
			window_.data(),
			static_cast<size_t>(fftSize_),
			juce::dsp::WindowingFunction<float>::hann,
			false);

		ring_.assign (static_cast<size_t>(fftSize_), 0.0f);
		ringWrite_ = 0;
		samplesSinceLastHop_ = 0;

		// JUCE's FFT performs in-place real-to-complex with size 2*fftSize_
		// in the buffer; allocate generously.
		frame_.assign (static_cast<size_t>(fftSize_ * 2), 0.0f);
		magnitudes_.assign (static_cast<size_t>(fftSize_ / 2 + 1), 0.0f);

		// 24 bands across 80 Hz..18 kHz keeps low rumble and air outside our
		// excitation range; the spec calls for ~24 ERB bands.
		numBands_ = 24;
		layout_   = makeErbLayout (numBands_, 80.0f, 18000.0f);
		binToBand_ = makeBinToBandMap (layout_, fftSize_, oversampledRate_);

		bandEnergyDb_.assign  (static_cast<size_t>(numBands_), -120.0f);
		bandGain_.assign       (static_cast<size_t>(numBands_), 0.0f);
		harshnessMask_.assign  (static_cast<size_t>(numBands_), 0.0f);
		bandFlatness_.assign   (static_cast<size_t>(numBands_), 0.5f);
		bandPow_.assign        (static_cast<size_t>(numBands_), 0.0f);
		bandCount_.assign      (static_cast<size_t>(numBands_), 0);

		// 5 ms attack, 80 ms release at the OS rate. Time per hop = hopSize / OSrate.
		const auto hopSec = static_cast<float>(hopSize_) / static_cast<float>(oversampledRate_);
		attackCoef_  = 1.0f - std::exp (-hopSec / 0.005f);
		releaseCoef_ = 1.0f - std::exp (-hopSec / 0.080f);

		// Latency: analysis itself only needs the lookahead of one window so
		// the wet path can be aligned with the analysis envelopes. For V1's
		// "wet drives gain modulation in the same STFT frame" architecture,
		// HarmonicGenerator already accounts for window/hop delay, so the
		// AnalysisBank itself reports zero. (HarmonicGenerator reports its
		// own latency separately.)
		latencySamples_ = 0;
	}

	void AnalysisBank::process (const juce::dsp::AudioBlock<float>& osBlock,
	                            float toneNorm,
	                            float sensitivityNorm)
	{
		toneNorm_        = toneNorm;
		sensitivityNorm_ = sensitivityNorm;

		const auto numCh = juce::jmin (numChannels_, static_cast<int>(osBlock.getNumChannels()));
		const auto N     = static_cast<int>(osBlock.getNumSamples());

		// Mix to mono and feed the ring buffer one sample at a time. For each
		// hopSize_ samples accumulated, emit one analysis hop.
		for (int i = 0; i < N; ++i)
		{
			float mono = 0.0f;
			for (int ch = 0; ch < numCh; ++ch)
				mono += osBlock.getSample (ch, i);
			mono *= (numCh > 0 ? 1.0f / static_cast<float>(numCh) : 0.0f);

			ring_[static_cast<size_t>(ringWrite_)] = mono;
			ringWrite_ = (ringWrite_ + 1) % fftSize_;

			if (++samplesSinceLastHop_ >= hopSize_)
			{
				samplesSinceLastHop_ = 0;
				runOneHop();
			}
		}
	}

	void AnalysisBank::runOneHop() noexcept
	{
		// Copy the latest fftSize_ samples out of the ring (oldest first) and
		// apply the Hann window into frame_.
		for (int i = 0; i < fftSize_; ++i)
		{
			const int idx = (ringWrite_ + i) % fftSize_;
			frame_[static_cast<size_t>(i)] = ring_[static_cast<size_t>(idx)] * window_[static_cast<size_t>(i)];
		}

		// Real-only FFT; JUCE's FFT does in-place real-to-complex and packs
		// the result interleaved; performFrequencyOnlyForwardTransform fills
		// magnitudes for us.
		std::fill (frame_.begin() + fftSize_, frame_.end(), 0.0f);
		fft_->performFrequencyOnlyForwardTransform (frame_.data());

		const int numBins = fftSize_ / 2 + 1;
		for (int b = 0; b < numBins; ++b)
			magnitudes_[static_cast<size_t>(b)] = frame_[static_cast<size_t>(b)];

		// Aggregate magnitudes per band (sum of |X|^2 -> dB). Use member
		// scratch buffers so the audio thread never allocates.
		std::fill (bandPow_.begin(),   bandPow_.end(),   0.0f);
		std::fill (bandCount_.begin(), bandCount_.end(), 0);
		for (int b = 0; b < numBins; ++b)
		{
			const int band = binToBand_[static_cast<size_t>(b)];
			if (band < 0 || band >= numBands_) continue;
			const float m = magnitudes_[static_cast<size_t>(b)];
			bandPow_[static_cast<size_t>(band)] += m * m;
			bandCount_[static_cast<size_t>(band)] += 1;
		}

		// Convert to dB. We keep flatness/tonality estimation deferred to a
		// post-V1 enhancement; the gate logic below only needs band energy.
		for (int k = 0; k < numBands_; ++k)
		{
			const float p = bandPow_[static_cast<size_t>(k)];
			const float dB = 10.0f * std::log10 (juce::jmax (p, 1.0e-12f));
			bandEnergyDb_[static_cast<size_t>(k)] = dB;
		}

		// Compute control envelopes. We use a simple rule set for V1:
		//
		//  bandGain[k] = saturate(sigmoid(bandDb[k] - threshold)
		//                         * neighborMaskFactor[k]
		//                         * toneWeight[k])
		//
		//  harshnessMask[k] is 1 in the 2-6 kHz region scaled by how loud
		//  that region is, 0 elsewhere.
		const float sensThresholdDb = juce::jmap (sensitivityNorm_, 0.0f, 1.0f, -20.0f, -55.0f);

		for (int k = 0; k < numBands_; ++k)
		{
			const float dB = bandEnergyDb_[static_cast<size_t>(k)];

			// Sigmoid gate: how active is this band relative to threshold?
			const float over = (dB - sensThresholdDb);
			float gate = 1.0f / (1.0f + std::exp (-over * 0.25f));

			// Masking: if neighbours are louder, the ear masks our addition,
			// so reduce. This is a simplification of true psychoacoustic
			// masking; it captures the dominant intuition cheaply.
			float neighbourDb = -120.0f;
			if (k > 0)
				neighbourDb = juce::jmax (neighbourDb, bandEnergyDb_[static_cast<size_t>(k - 1)]);
			if (k < numBands_ - 1)
				neighbourDb = juce::jmax (neighbourDb, bandEnergyDb_[static_cast<size_t>(k + 1)]);
			const float masked = juce::jlimit (0.0f, 1.0f, 1.0f - juce::jmax (0.0f, neighbourDb - dB) / 20.0f);

			// Tone weighting: bias gate toward darker (lower-band) or brighter
			// (higher-band) regions linearly across band index.
			const float t = static_cast<float>(k) / static_cast<float>(juce::jmax (1, numBands_ - 1));
			const float toneWeight = 1.0f + toneNorm_ * (t - 0.5f);

			const float target = juce::jlimit (0.0f, 1.0f, gate * masked * toneWeight);

			float& g = bandGain_[static_cast<size_t>(k)];
			const float coef = (target > g) ? attackCoef_ : releaseCoef_;
			g += coef * (target - g);

			// Harshness mask: scale up in the 2-6 kHz "ear-pain" zone when
			// in-band energy is already high.
			const float fc = layout_.centerHz[static_cast<size_t>(k)];
			float harshTarget = 0.0f;
			if (fc >= 2000.0f && fc <= 6000.0f)
			{
				const float over2 = juce::jmax (0.0f, dB - (-30.0f));
				harshTarget = juce::jlimit (0.0f, 1.0f, over2 / 20.0f);
			}
			float& h = harshnessMask_[static_cast<size_t>(k)];
			h += (harshTarget > h ? attackCoef_ : releaseCoef_) * (harshTarget - h);
		}
	}

	void AnalysisBank::fillActivityFrame (ActivityFrame& frame) const noexcept
	{
		const int n = juce::jmin (numBands_, static_cast<int>(kMaxActivityBands));
		frame.numBands = n;
		for (int k = 0; k < n; ++k)
		{
			frame.bandGain[static_cast<size_t>(k)]  = bandGain_[static_cast<size_t>(k)];
			frame.harshness[static_cast<size_t>(k)] = harshnessMask_[static_cast<size_t>(k)];
		}
	}
}
