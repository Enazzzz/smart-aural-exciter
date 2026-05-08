#pragma once

// Single source of truth for all plugin parameters.
//
// Parameter IDs, ranges, defaults, and labels live here so that the DSP layer
// (PluginProcessor / DspEngine) and the UI layer (PluginEditor / panels) both
// read identical metadata. If a value disagrees with docs/spec/01-scope-freeze.md
// section 5.2 / 5.3, this file is wrong - the spec wins.

#include <juce_audio_processors/juce_audio_processors.h>

namespace smex
{
	// String IDs are short and stable; do NOT rename without bumping a state
	// version, or saved REAPER projects will lose their parameter automation.
	namespace ParamId
	{
		inline constexpr const char* amount        = "amount";
		inline constexpr const char* tone          = "tone";
		inline constexpr const char* sensitivity   = "sensitivity";
		inline constexpr const char* mix           = "mix";
		inline constexpr const char* output        = "output";

		inline constexpr const char* qualityMode   = "qualityMode";
		inline constexpr const char* guardAmount   = "guardAmount";
		inline constexpr const char* activityScale = "activityScale";
		inline constexpr const char* headroomTrim  = "headroomTrim";
	}

	// Quality mode values match the order in PluginEditor's combo box. Standard
	// is the safer/lower-latency path, High maximizes audible fidelity at the
	// cost of latency (see docs/spec/02-measurements.md section 3).
	enum class QualityMode : int
	{
		Standard = 0,
		High     = 1
	};

	enum class ActivityScale : int
	{
		Linear = 0,
		Log    = 1,
		Auto   = 2
	};

	// Build the JUCE APVTS layout exactly as locked by the scope freeze.
	juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

	// Convenience accessor: returns the current QualityMode atomically. Reading
	// the raw float and comparing to integer literals everywhere is ugly.
	QualityMode getQualityMode (const juce::AudioProcessorValueTreeState& apvts);

	ActivityScale getActivityScale (const juce::AudioProcessorValueTreeState& apvts);
}
