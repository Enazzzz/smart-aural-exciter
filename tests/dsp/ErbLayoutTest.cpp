// Test: ERB band layout sanity
// ----------------------------
// Verifies that the ERB band layout produces monotonically increasing edges
// and that every FFT bin maps to a valid band index.

#include <iostream>

#include "dsp/ErbBands.h"

int main()
{
	const auto layout = smex::makeErbLayout (24, 80.0f, 18000.0f);
	if (layout.numBands != 24) { std::cerr << "numBands wrong\n"; return 1; }

	for (size_t i = 1; i < layout.edgeHz.size(); ++i)
	{
		if (layout.edgeHz[i] <= layout.edgeHz[i - 1])
		{
			std::cerr << "Band edges not monotonic at " << i << ": "
			          << layout.edgeHz[i - 1] << " >= " << layout.edgeHz[i] << "\n";
			return 2;
		}
	}

	const int fftSize = 1024;
	const auto map = smex::makeBinToBandMap (layout, fftSize, 96000.0);
	for (int b = 0; b < fftSize / 2 + 1; ++b)
	{
		const int band = map[static_cast<size_t>(b)];
		if (band < 0 || band >= layout.numBands)
		{
			std::cerr << "Bin " << b << " mapped to invalid band " << band << "\n";
			return 3;
		}
	}

	std::cout << "OK: ERB layout monotonic, all bins mapped.\n";
	return 0;
}
