#include "SafetyLimiter.h"

#include <cmath>

namespace smex
{
	SafetyLimiter::SafetyLimiter() = default;

	void SafetyLimiter::prepare (double sampleRate, int maxBlockSize, int numChannels)
	{
		juce::ignoreUnused (maxBlockSize);
		sampleRate_   = sampleRate;
		numChannels_  = juce::jlimit (1, 2, numChannels);

		// 30 ms release - inaudible on transient material; fast enough that
		// program material recovers cleanly between events.
		const float releaseSec = 0.030f;
		releaseCoef_ = std::exp (-1.0f / (releaseSec * static_cast<float>(sampleRate_)));

		gainState_[0] = gainState_[1] = 1.0f;
	}

	void SafetyLimiter::process (juce::AudioBuffer<float>& buffer)
	{
		const auto numCh = juce::jmin (numChannels_, buffer.getNumChannels());
		const auto numSamples = buffer.getNumSamples();

		for (int ch = 0; ch < numCh; ++ch)
		{
			auto* d = buffer.getWritePointer (ch);
			float gain = gainState_[ch];

			for (int i = 0; i < numSamples; ++i)
			{
				const float x = d[i];
				const float ax = std::abs (x);

				// NaN/Inf safeguard: if upstream produced garbage, swap in 0.
				const float xClean = std::isfinite (x) ? x : 0.0f;
				const float axClean = std::isfinite (ax) ? ax : 0.0f;

				// Compute target gain. If the absolute sample exceeds the
				// ceiling, target = ceiling/abs. If between knee start and
				// ceiling, soft-knee interpolates. Otherwise target = 1.
				float target;
				if (axClean <= kKneeStart)
				{
					target = 1.0f;
				}
				else if (axClean >= kCeilingLin)
				{
					target = kCeilingLin / axClean;
				}
				else
				{
					// Quadratic soft-knee inside [kKneeStart .. kCeilingLin]:
					// at the lower edge gain = 1, at the upper edge gain = ceiling/x.
					const float t = (axClean - kKneeStart) / (kCeilingLin - kKneeStart);
					const float upperGain = kCeilingLin / axClean;
					target = 1.0f + t * t * (upperGain - 1.0f);
				}

				// Attack is instantaneous so transients can never break the
				// ceiling; release is exponential per the prepared coefficient.
				if (target < gain) gain = target;
				else               gain = target + (gain - target) * releaseCoef_;

				d[i] = xClean * gain;
			}

			gainState_[ch] = gain;
		}
	}
}
