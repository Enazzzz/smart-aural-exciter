#include "Parameters.h"

namespace smex
{
	using APF = juce::AudioParameterFloat;
	using APC = juce::AudioParameterChoice;

	juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
	{
		// Version hint = 1: bump if the parameter set changes shape across releases
		// so JUCE can reject incompatible saved state instead of silently mapping it.
		juce::AudioProcessorValueTreeState::ParameterLayout layout;

		// Simple panel ------------------------------------------------------
		// Amount: global excitation intensity. 0..100 with default 35 per the
		// spec freeze; mapping to internal gain happens in HarmonicGenerator.
		layout.add (std::make_unique<APF>(
			juce::ParameterID { ParamId::amount, 1 },
			"Amount",
			juce::NormalisableRange<float> { 0.0f, 100.0f, 0.01f },
			35.0f));

		// Tone: tilts the band-weighting bias darker (-) / brighter (+).
		layout.add (std::make_unique<APF>(
			juce::ParameterID { ParamId::tone, 1 },
			"Tone",
			juce::NormalisableRange<float> { -50.0f, 50.0f, 0.01f },
			0.0f));

		// Sensitivity: lowers the analysis threshold (more bands open faster).
		layout.add (std::make_unique<APF>(
			juce::ParameterID { ParamId::sensitivity, 1 },
			"Sensitivity",
			juce::NormalisableRange<float> { 0.0f, 100.0f, 0.01f },
			50.0f));

		// Mix: dry/wet blend in percent (100 = fully wet, V1 default).
		layout.add (std::make_unique<APF>(
			juce::ParameterID { ParamId::mix, 1 },
			"Mix",
			juce::NormalisableRange<float> { 0.0f, 100.0f, 0.01f },
			100.0f));

		// Output: post-process trim in dB before the safety stage.
		layout.add (std::make_unique<APF>(
			juce::ParameterID { ParamId::output, 1 },
			"Output",
			juce::NormalisableRange<float> { -12.0f, 6.0f, 0.01f },
			0.0f));

		// Advanced panel ----------------------------------------------------
		layout.add (std::make_unique<APC>(
			juce::ParameterID { ParamId::qualityMode, 1 },
			"Quality",
			juce::StringArray { "Standard", "High" },
			static_cast<int>(QualityMode::Standard)));

		layout.add (std::make_unique<APF>(
			juce::ParameterID { ParamId::guardAmount, 1 },
			"Guard",
			juce::NormalisableRange<float> { 0.0f, 100.0f, 0.01f },
			60.0f));

		layout.add (std::make_unique<APC>(
			juce::ParameterID { ParamId::activityScale, 1 },
			"Activity Scale",
			juce::StringArray { "Linear", "Log", "Auto" },
			static_cast<int>(ActivityScale::Auto)));

		// Headroom trim sits between the wet sum and the output gain so the
		// safety limiter has consistent room to work; default -3 dB per spec.
		layout.add (std::make_unique<APF>(
			juce::ParameterID { ParamId::headroomTrim, 1 },
			"Headroom",
			juce::NormalisableRange<float> { -6.0f, 0.0f, 0.01f },
			-3.0f));

		return layout;
	}

	QualityMode getQualityMode (const juce::AudioProcessorValueTreeState& apvts)
	{
		// AudioParameterChoice exposes the current index as a normalized float,
		// which is awkward; load the underlying parameter and read its index.
		if (auto* p = apvts.getParameter (ParamId::qualityMode))
		{
			const auto idx = static_cast<int>(p->convertFrom0to1 (p->getValue()));
			return idx == static_cast<int>(QualityMode::High)
				? QualityMode::High
				: QualityMode::Standard;
		}
		return QualityMode::Standard;
	}

	ActivityScale getActivityScale (const juce::AudioProcessorValueTreeState& apvts)
	{
		if (auto* p = apvts.getParameter (ParamId::activityScale))
		{
			const auto idx = static_cast<int>(p->convertFrom0to1 (p->getValue()));
			switch (idx)
			{
				case static_cast<int>(ActivityScale::Linear): return ActivityScale::Linear;
				case static_cast<int>(ActivityScale::Log):    return ActivityScale::Log;
				default:                                       return ActivityScale::Auto;
			}
		}
		return ActivityScale::Auto;
	}
}
