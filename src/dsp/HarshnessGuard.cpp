#include "HarshnessGuard.h"

#include <algorithm>

namespace smex
{
	HarshnessGuard::HarshnessGuard() = default;

	void HarshnessGuard::prepare (int numBands)
	{
		numBands_ = numBands;
		atten_.assign (static_cast<size_t>(numBands_), 1.0f);
	}

	void HarshnessGuard::process (const float* harshnessMask, int numBands, float guardAmount)
	{
		const int n = std::min (numBands, numBands_);

		// Effective guard amount is the user setting clamped to [floor, 1.0]
		// so even guardAmount=0 keeps a small protective ceiling.
		const float effective = std::max (kMinGuardFloor, guardAmount);

		for (int k = 0; k < n; ++k)
		{
			const float mask = harshnessMask[k];                  // 0..1
			const float a    = 1.0f - effective * mask;            // 0..1
			atten_[static_cast<size_t>(k)] = a < 0.0f ? 0.0f : a;
		}
	}
}
