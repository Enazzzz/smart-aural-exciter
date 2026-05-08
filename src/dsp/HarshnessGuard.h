#pragma once

// HarshnessGuard
// --------------
// Computes per-band attenuation coefficients that protect the listener from
// ice-pick excitation (docs/spec/01-scope-freeze.md section 4 - "forbidden
// side effects"). The actual audio attenuation happens inside
// HarmonicGenerator's FFT pass, which multiplies bandGain by the guard's
// attenuation per band. Keeping the guard as a coefficient producer avoids
// running a second FFT/IFFT just for attenuation.
//
// A small minimum guard floor (kMinGuardFloor) is always applied even when
// the advanced Guard parameter is 0, so the listener is never fully exposed
// to the worst-case harshness.

#include <vector>

namespace smex
{
	class HarshnessGuard
	{
	public:
		HarshnessGuard();

		void prepare (int numBands);

		// Compute attenuation = 1 - effectiveGuard * harshnessMask per band.
		// Stores results internally for HarmonicGenerator to read.
		void process (const float* harshnessMask, int numBands, float guardAmount);

		const float* getAttenuation() const noexcept { return atten_.data(); }
		int          getNumBands()    const noexcept { return numBands_; }

		int getLatencySamples() const noexcept { return 0; }

	private:
		// Minimum protection that is always active so the listener is never
		// fully exposed to the worst-case harshness even if the user sets
		// Guard to 0.
		static constexpr float kMinGuardFloor = 0.15f;

		int numBands_ { 0 };
		std::vector<float> atten_;
	};
}
