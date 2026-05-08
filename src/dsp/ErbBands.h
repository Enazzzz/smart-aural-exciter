#pragma once

// ErbBands
// --------
// Utility for laying out perceptual band edges on the ERB scale and mapping
// FFT bins onto those bands. ERB (Equivalent Rectangular Bandwidth) is the
// psychoacoustic scale we use because the spec calls for "perceptual" rather
// than linear-frequency band layout (docs/spec/03-dsp-architecture.md §3.3).
//
// The classic Glasberg-Moore formulas:
//   ERB(f) = 24.7 * (4.37 * f/1000 + 1)             [Hz]
//   ERBs from f1 to f2: 21.4 * log10(4.37*f/1000 + 1)
//
// We invert that to lay out N bands evenly in ERB space between fLow and fHigh.

#include <vector>

namespace smex
{
	struct ErbLayout
	{
		// edgeHz has size numBands+1 - bin i covers [edgeHz[i] .. edgeHz[i+1]).
		std::vector<float> edgeHz;
		// centerHz has size numBands - geometric center of each band.
		std::vector<float> centerHz;
		int numBands { 0 };
	};

	// Build an ERB-spaced band layout between fLow and fHigh.
	ErbLayout makeErbLayout (int numBands, float fLow, float fHigh);

	// Map FFT bin index to band index. The result is precomputed once at
	// prepare-time so the per-block hot path never recomputes it.
	std::vector<int> makeBinToBandMap (const ErbLayout& layout,
	                                   int fftSize,
	                                   double sampleRate);
}
