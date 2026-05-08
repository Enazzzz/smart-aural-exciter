#include "MeterTap.h"

#include <cmath>

namespace smex
{
	MeterTap::MeterTap() = default;

	void MeterTap::prepare (double sampleRate)
	{
		sampleRate_ = sampleRate;
	}

	static float bufferRms (const juce::AudioBuffer<float>& b, int ch)
	{
		if (ch >= b.getNumChannels()) return 0.0f;
		const auto* d = b.getReadPointer (ch);
		const auto N = b.getNumSamples();
		double sum = 0.0;
		for (int i = 0; i < N; ++i)
			sum += static_cast<double>(d[i]) * d[i];
		return N > 0 ? static_cast<float>(std::sqrt (sum / N)) : 0.0f;
	}

	void MeterTap::captureInputAndOutput (const juce::AudioBuffer<float>& in,
	                                      const juce::AudioBuffer<float>& out) noexcept
	{
		// We publish per-block peaks/RMS values; the UI handles ballistics
		// (decay, hold) so the audio thread stays cheap.
		inPeakL_.store  (in.getMagnitude (0, 0, in.getNumSamples()),  std::memory_order_relaxed);
		inPeakR_.store  (in.getNumChannels() > 1
			? in.getMagnitude  (1, 0, in.getNumSamples()) : 0.0f,     std::memory_order_relaxed);
		outPeakL_.store (out.getMagnitude (0, 0, out.getNumSamples()), std::memory_order_relaxed);
		outPeakR_.store (out.getNumChannels() > 1
			? out.getMagnitude (1, 0, out.getNumSamples()) : 0.0f,    std::memory_order_relaxed);
		inRmsL_.store   (bufferRms (in,  0),                           std::memory_order_relaxed);
		outRmsL_.store  (bufferRms (out, 0),                           std::memory_order_relaxed);
	}

	MeterSnapshot MeterTap::snapshot() const noexcept
	{
		MeterSnapshot s;
		s.inPeakL  = inPeakL_.load (std::memory_order_relaxed);
		s.inPeakR  = inPeakR_.load (std::memory_order_relaxed);
		s.outPeakL = outPeakL_.load (std::memory_order_relaxed);
		s.outPeakR = outPeakR_.load (std::memory_order_relaxed);
		s.inRmsL   = inRmsL_.load (std::memory_order_relaxed);
		s.outRmsL  = outRmsL_.load (std::memory_order_relaxed);
		return s;
	}
}
