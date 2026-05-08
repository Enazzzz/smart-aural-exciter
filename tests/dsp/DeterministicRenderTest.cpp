// Test: deterministic offline render
// ----------------------------------
// Renders the same input twice through DspEngine and asserts the outputs are
// bit-identical. Required by docs/spec/02-measurements.md section 7.
//
// Failure here means we've added something stateful or thread-dependent to
// the audio path that breaks reproducibility.

#include <iostream>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

#include "dsp/DspEngine.h"

namespace
{
	void fillTestSignal (juce::AudioBuffer<float>& buf)
	{
		// Mix of a 1 kHz sine and band-limited noise gives both tonal and
		// noisy content for the analysis bank to chew on.
		const auto N = buf.getNumSamples();
		const float fs = 48000.0f;
		const float f  = 1000.0f;
		const float twoPi = juce::MathConstants<float>::twoPi;

		juce::Random rng (42);
		for (int ch = 0; ch < buf.getNumChannels(); ++ch)
		{
			auto* d = buf.getWritePointer (ch);
			for (int i = 0; i < N; ++i)
				d[i] = 0.5f * std::sin (twoPi * f * static_cast<float>(i) / fs)
				     + 0.05f * (rng.nextFloat() * 2.0f - 1.0f);
		}
	}

	// Render one full pass and store output samples into `out`.
	void renderPass (std::vector<float>& outL, std::vector<float>& outR)
	{
		smex::DspEngine engine;
		engine.prepare (48000.0, 256, 2);

		// Default-equivalent params (matching scope-freeze defaults).
		smex::DspEngine::LiveParams p;
		p.amount = 0.35f;
		p.tone = 0.0f;
		p.sensitivity = 0.5f;
		p.mix = 1.0f;
		p.outputGain = 1.0f;
		p.guardAmount = 0.6f;
		p.headroomGain = juce::Decibels::decibelsToGain (-3.0f);
		engine.setParams (p);

		const int totalBlocks = 100;
		const int blockSize   = 256;
		outL.reserve (static_cast<size_t>(totalBlocks * blockSize));
		outR.reserve (static_cast<size_t>(totalBlocks * blockSize));

		juce::AudioBuffer<float> buf (2, blockSize);
		for (int b = 0; b < totalBlocks; ++b)
		{
			fillTestSignal (buf);
			engine.process (buf, false);
			for (int i = 0; i < blockSize; ++i)
			{
				outL.push_back (buf.getSample (0, i));
				outR.push_back (buf.getSample (1, i));
			}
		}
	}
}

int main()
{
	std::vector<float> aL, aR, bL, bR;
	renderPass (aL, aR);
	renderPass (bL, bR);

	if (aL.size() != bL.size() || aR.size() != bR.size())
	{
		std::cerr << "Size mismatch between runs.\n";
		return 1;
	}

	for (size_t i = 0; i < aL.size(); ++i)
	{
		if (aL[i] != bL[i] || aR[i] != bR[i])
		{
			std::cerr << "Determinism violation at sample " << i
			          << ": L (" << aL[i] << " vs " << bL[i] << "), "
			          << " R (" << aR[i] << " vs " << bR[i] << ")\n";
			return 2;
		}
	}

	std::cout << "OK: " << aL.size() << " samples bit-identical across runs.\n";
	return 0;
}
