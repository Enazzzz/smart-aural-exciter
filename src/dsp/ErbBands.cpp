#include "ErbBands.h"

#include <algorithm>
#include <cmath>

namespace smex
{
	// Tiny clamp helper - kept local so this file doesn't depend on juce_core.
	static int clampi (int v, int lo, int hi) noexcept
	{
		return v < lo ? lo : (v > hi ? hi : v);
	}

	// ERBs from 0 to f Hz on the Glasberg-Moore scale.
	static float hzToErbs (float f)
	{
		return 21.4f * std::log10 (4.37f * f / 1000.0f + 1.0f);
	}

	// Inverse of hzToErbs: returns frequency in Hz for a given number of ERBs.
	static float erbsToHz (float erbs)
	{
		const float ratio = std::pow (10.0f, erbs / 21.4f) - 1.0f;
		return ratio * 1000.0f / 4.37f;
	}

	ErbLayout makeErbLayout (int numBands, float fLow, float fHigh)
	{
		ErbLayout out;
		if (numBands < 1) return out;

		out.numBands = numBands;
		out.edgeHz.resize (static_cast<size_t>(numBands + 1));
		out.centerHz.resize (static_cast<size_t>(numBands));

		const float erbLo = hzToErbs (fLow);
		const float erbHi = hzToErbs (fHigh);
		const float step  = (erbHi - erbLo) / static_cast<float>(numBands);

		for (int i = 0; i <= numBands; ++i)
			out.edgeHz[static_cast<size_t>(i)] = erbsToHz (erbLo + step * static_cast<float>(i));

		for (int i = 0; i < numBands; ++i)
		{
			// Geometric center for log-friendly visualisation.
			out.centerHz[static_cast<size_t>(i)] = std::sqrt (
				out.edgeHz[static_cast<size_t>(i)] *
				out.edgeHz[static_cast<size_t>(i + 1)]);
		}

		return out;
	}

	std::vector<int> makeBinToBandMap (const ErbLayout& layout,
	                                   int fftSize,
	                                   double sampleRate)
	{
		const int numBins = fftSize / 2 + 1;
		std::vector<int> map (static_cast<size_t>(numBins), -1);

		const float binHz = static_cast<float>(sampleRate) / static_cast<float>(fftSize);

		for (int bin = 0; bin < numBins; ++bin)
		{
			const float f = static_cast<float>(bin) * binHz;
			if (f < layout.edgeHz.front()) { map[static_cast<size_t>(bin)] = 0; continue; }
			if (f >= layout.edgeHz.back()) { map[static_cast<size_t>(bin)] = layout.numBands - 1; continue; }

			// Find the first edge greater than f - the band index is one less.
			auto it = std::upper_bound (layout.edgeHz.begin(), layout.edgeHz.end(), f);
			const int band = clampi (
				static_cast<int>(std::distance (layout.edgeHz.begin(), it)) - 1,
				0, layout.numBands - 1);
			map[static_cast<size_t>(bin)] = band;
		}

		return map;
	}
}
