#include "ActivityView.h"

#include <cmath>

namespace smex
{
	ActivityView::ActivityView() = default;

	static float scaleValue (float v, ActivityScale scale)
	{
		switch (scale)
		{
			case ActivityScale::Linear:
				return juce::jlimit (0.0f, 1.0f, v);
			case ActivityScale::Log:
			{
				const float dB = 20.0f * std::log10 (juce::jmax (1.0e-3f, v));
				return juce::jlimit (0.0f, 1.0f, (dB + 60.0f) / 60.0f);
			}
			case ActivityScale::Auto:
			default:
			{
				// "Auto" picks log when v is small, linear when v is large -
				// this gives both ends of the range readable visual response.
				if (v < 0.1f) return scaleValue (v, ActivityScale::Log);
				return scaleValue (v, ActivityScale::Linear);
			}
		}
	}

	void ActivityView::paint (juce::Graphics& g)
	{
		auto r = getLocalBounds().reduced (4);

		g.setColour (juce::Colours::white.withAlpha (0.05f));
		g.fillRoundedRectangle (r.toFloat(), 6.0f);

		const int n = frame_.numBands;
		if (n <= 0) return;

		const float barW = static_cast<float>(r.getWidth()) / static_cast<float>(n);

		for (int k = 0; k < n; ++k)
		{
			const float gain = scaleValue (frame_.bandGain[static_cast<size_t>(k)], scale_);
			const float harsh = scaleValue (frame_.harshness[static_cast<size_t>(k)], scale_);

			const auto x = r.getX() + barW * static_cast<float>(k);
			const auto h = r.getHeight() * gain;
			const juce::Rectangle<float> bar { x + 1.0f, r.getBottom() - h,
			                                    barW - 2.0f, h };

			g.setColour (juce::Colours::cyan.withAlpha (0.55f));
			g.fillRect (bar);

			// Overlay a small red marker showing the harshness mask intensity
			// so the user can see WHY excitation is being suppressed.
			if (harsh > 0.01f)
			{
				const auto markerH = juce::jlimit (1.0f, r.getHeight() * 0.05f,
				                                    r.getHeight() * harsh * 0.05f);
				const juce::Rectangle<float> marker { x + 1.0f, r.getY() + markerH,
				                                       barW - 2.0f, markerH };
				g.setColour (juce::Colours::red.withAlpha (0.75f));
				g.fillRect (marker);
			}
		}
	}
}
