#pragma once

// ActivityView
// ------------
// Band-resolved visualization of where the harmonic generator is currently
// adding emphasis (docs/spec/01-scope-freeze.md section 5.4 - "enhancement
// activity view"). Reads from ActivityFrame snapshots; refresh rate is
// driven by the editor's 30 Hz timer.

#include <juce_gui_basics/juce_gui_basics.h>
#include "../dsp/ActivitySnapshot.h"
#include "../plugin/Parameters.h"

namespace smex
{
	class ActivityView : public juce::Component
	{
	public:
		ActivityView();

		void setSnapshot (const ActivityFrame& f, ActivityScale scale) noexcept
		{
			frame_ = f;
			scale_ = scale;
		}

		void paint (juce::Graphics& g) override;

	private:
		ActivityFrame frame_ {};
		ActivityScale scale_ { ActivityScale::Auto };
	};
}
