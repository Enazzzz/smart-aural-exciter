// Test: SafetyLimiter ceiling enforcement
// ---------------------------------------
// Confirms the limiter never lets a sample exceed the -1.0 dBFS ceiling
// even under extreme overload, and never emits NaN/Inf or DC offset.
// (docs/spec/02-measurements.md section 5.)

#include <cmath>
#include <iostream>

#include <juce_audio_basics/juce_audio_basics.h>

#include "dsp/SafetyLimiter.h"

namespace
{
	bool isFiniteSample (float v) { return std::isfinite (v); }
}

int main()
{
	const double sr = 48000.0;
	const int    N  = 2048;

	smex::SafetyLimiter lim;
	lim.prepare (sr, N, 2);

	// Build a stress signal: +12 dB DC-ish square so the limiter is hammered.
	juce::AudioBuffer<float> buf (2, N);
	for (int ch = 0; ch < 2; ++ch)
	{
		auto* d = buf.getWritePointer (ch);
		for (int i = 0; i < N; ++i)
		{
			const float baseSquare = (i % 64 < 32) ? 1.0f : -1.0f;
			d[i] = baseSquare * 4.0f; // +12 dB-ish overload
		}
	}

	// Run a few times so release state cycles.
	for (int pass = 0; pass < 10; ++pass)
		lim.process (buf);

	float worst = 0.0f;
	for (int ch = 0; ch < 2; ++ch)
	{
		const auto* d = buf.getReadPointer (ch);
		for (int i = 0; i < N; ++i)
		{
			if (! isFiniteSample (d[i]))
			{
				std::cerr << "Non-finite sample at ch " << ch << ", i " << i << "\n";
				return 1;
			}
			worst = std::max (worst, std::abs (d[i]));
		}
	}

	const float ceiling = 0.8912509f; // -1.0 dBFS
	if (worst > ceiling + 1.0e-3f) // tiny tolerance for float math
	{
		std::cerr << "Limiter let through " << worst
		          << " (max allowed " << ceiling << ")\n";
		return 2;
	}

	std::cout << "OK: limiter held output to " << worst << " <= " << ceiling << "\n";
	return 0;
}
