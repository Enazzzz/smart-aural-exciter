#include "Oversampler.h"

namespace smex
{
	Oversampler::Oversampler() = default;

	void Oversampler::prepare (double hostSampleRate, int maxBlockSize, int numChannels)
	{
		hostSampleRate_ = hostSampleRate;
		maxBlockSize_   = maxBlockSize;
		numChannels_    = juce::jlimit (1, 2, numChannels);

		using OS = juce::dsp::Oversampling<float>;

		// Polyphase IIR is the lowest-latency choice and CPU-friendly. The DSP
		// architecture spec lists linear-phase FIR as the eventual target;
		// for V1 we accept this trade-off and revisit if null-tests show
		// audible phase artifacts.
		osStandard_ = std::make_unique<OS>(
			static_cast<size_t>(numChannels_),
			1,
			OS::filterHalfBandPolyphaseIIR,
			false);

		osHigh_ = std::make_unique<OS>(
			static_cast<size_t>(numChannels_),
			2,
			OS::filterHalfBandPolyphaseIIR,
			false);

		osStandard_->initProcessing (static_cast<size_t>(maxBlockSize_));
		osHigh_->initProcessing     (static_cast<size_t>(maxBlockSize_));

		// Dry tap lives at the largest possible OS rate (4x for High mode),
		// sized once here and never re-allocated on the audio thread.
		dryTap_.setSize (numChannels_, maxBlockSize_ * 4, false, true, true);
		dryTapValidSamples_ = 0;
	}

	void Oversampler::setMode (QualityMode mode)
	{
		mode_ = mode;
		if (osStandard_) osStandard_->reset();
		if (osHigh_)     osHigh_->reset();
	}

	int Oversampler::getLatencySamples() const noexcept
	{
		if (auto* c = current())
			return static_cast<int>(c->getLatencyInSamples());
		return 0;
	}

	double Oversampler::getOversampledRate() const noexcept
	{
		const auto factor = mode_ == QualityMode::High ? 4 : 2;
		return hostSampleRate_ * factor;
	}

	int Oversampler::getMaxOversampledBlock() const noexcept
	{
		const auto factor = mode_ == QualityMode::High ? 4 : 2;
		return maxBlockSize_ * factor;
	}

	juce::dsp::AudioBlock<float> Oversampler::processUp (const juce::AudioBuffer<float>& host)
	{
		const auto numCh = juce::jmin (numChannels_, host.getNumChannels());

		// Build a non-const pointer view onto host's existing storage. JUCE's
		// processSamplesUp does not actually modify the input, even though
		// the AudioBlock constructor isn't const-aware.
		juce::dsp::AudioBlock<float> hostBlock {
			const_cast<float**>(host.getArrayOfReadPointers()),
			static_cast<size_t>(numCh),
			static_cast<size_t>(host.getNumSamples())
		};

		auto* os = current();
		auto upBlock = os->processSamplesUp (hostBlock);

		// Capture dry tap from the up-rate block before the engine modifies it.
		const auto upN = static_cast<int>(upBlock.getNumSamples());
		dryTapValidSamples_ = upN;
		for (int ch = 0; ch < numCh; ++ch)
		{
			const auto* src = upBlock.getChannelPointer (static_cast<size_t>(ch));
			juce::FloatVectorOperations::copy (dryTap_.getWritePointer (ch), src, upN);
		}

		return upBlock;
	}

	juce::dsp::AudioBlock<float> Oversampler::getDryBlock() noexcept
	{
		return juce::dsp::AudioBlock<float> {
			dryTap_.getArrayOfWritePointers(),
			static_cast<size_t>(numChannels_),
			static_cast<size_t>(dryTapValidSamples_)
		};
	}

	void Oversampler::processDown (juce::AudioBuffer<float>& host)
	{
		auto* os = current();
		juce::dsp::AudioBlock<float> hostBlock { host };
		os->processSamplesDown (hostBlock);
	}

	void Oversampler::processBypassed (juce::AudioBuffer<float>& host)
	{
		auto* os = current();
		juce::dsp::AudioBlock<float> hostBlock { host };
		auto up = os->processSamplesUp (hostBlock);
		juce::ignoreUnused (up);
		os->processSamplesDown (hostBlock);
	}
}
