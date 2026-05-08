#include "MeterView.h"

#include <cmath>

namespace smex
{
	MeterView::MeterView() = default;

	void MeterView::drawBar (juce::Graphics& g, juce::Rectangle<int> r,
	                          float linAmplitude, const juce::String& caption)
	{
		// Background well.
		g.setColour (juce::Colours::white.withAlpha (0.05f));
		g.fillRoundedRectangle (r.toFloat(), 3.0f);

		// Convert linear amplitude to dBFS (0 dBFS = top), clamped to -60.
		const float dB = 20.0f * std::log10 (juce::jmax (1.0e-3f, linAmplitude));
		const float t = juce::jlimit (0.0f, 1.0f, (dB + 60.0f) / 60.0f);

		auto fill = r.withTrimmedTop (static_cast<int>(r.getHeight() * (1.0f - t)));
		g.setColour (juce::Colours::limegreen.withAlpha (0.7f));
		g.fillRoundedRectangle (fill.toFloat(), 3.0f);

		g.setColour (juce::Colours::white);
		g.setFont (10.0f);
		g.drawText (caption, r.withHeight (12), juce::Justification::centred);
	}

	void MeterView::paint (juce::Graphics& g)
	{
		// UI-side ballistics: fast attack, slow decay, so peak excursions hold
		// briefly enough to see at 30 Hz refresh.
		const float decay = 0.85f;
		auto smooth = [decay](float& d, float src)
		{
			if (src > d) d = src;
			else         d = src + (d - src) * decay;
		};
		smooth (displayInL_,  latest_.inPeakL);
		smooth (displayInR_,  latest_.inPeakR);
		smooth (displayOutL_, latest_.outPeakL);
		smooth (displayOutR_, latest_.outPeakR);

		auto r = getLocalBounds().reduced (4);
		const int barW = (r.getWidth() - 12) / 4;

		drawBar (g, r.removeFromLeft (barW),       displayInL_,  "InL");
		r.removeFromLeft (4);
		drawBar (g, r.removeFromLeft (barW),       displayInR_,  "InR");
		r.removeFromLeft (4);
		drawBar (g, r.removeFromLeft (barW),       displayOutL_, "OutL");
		r.removeFromLeft (4);
		drawBar (g, r,                              displayOutR_, "OutR");
	}
}
