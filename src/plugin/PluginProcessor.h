#pragma once

// PluginProcessor
// ---------------
// JUCE AudioProcessor entry point. Owns the APVTS, the DspEngine, the
// editor, and the host-facing things JUCE requires (state save/load,
// latency reporting, bypass behavior).
//
// Responsibilities are deliberately thin: this class wires APVTS values
// into DspEngine::LiveParams once per block, then delegates audio work to
// the engine. All DSP lives in src/dsp/.

#include <juce_audio_processors/juce_audio_processors.h>

#include "Parameters.h"
#include "../dsp/DspEngine.h"

namespace smex
{
	class PluginProcessor : public juce::AudioProcessor
	{
	public:
		PluginProcessor();
		~PluginProcessor() override = default;

		void prepareToPlay (double sampleRate, int samplesPerBlock) override;
		void releaseResources() override;

		bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

		void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;
		using juce::AudioProcessor::processBlock;

		juce::AudioProcessorEditor* createEditor() override;
		bool hasEditor() const override { return true; }

		const juce::String getName() const override { return JucePlugin_Name; }

		bool acceptsMidi() const override               { return false; }
		bool producesMidi() const override              { return false; }
		bool isMidiEffect() const override              { return false; }
		double getTailLengthSeconds() const override    { return 0.0; }

		int getNumPrograms() override                   { return 1; }
		int getCurrentProgram() override                { return 0; }
		void setCurrentProgram (int) override           {}
		const juce::String getProgramName (int) override { return "Default"; }
		void changeProgramName (int, const juce::String&) override {}

		void getStateInformation (juce::MemoryBlock& destData) override;
		void setStateInformation (const void* data, int sizeInBytes) override;

		// Exposed so the editor can bind to params and read engine snapshots.
		juce::AudioProcessorValueTreeState& getApvts() noexcept { return apvts_; }
		DspEngine&                           getEngine() noexcept { return engine_; }

	private:
		// Pull current parameter values out of APVTS and translate them into
		// the engine's internal float ranges. Called once per block.
		void pushParamsIntoEngine();

		juce::AudioProcessorValueTreeState apvts_;
		DspEngine engine_;

		// Cached raw parameter pointers - looking up by string ID per block
		// would be wasteful.
		juce::AudioParameterFloat*  pAmount_       { nullptr };
		juce::AudioParameterFloat*  pTone_         { nullptr };
		juce::AudioParameterFloat*  pSensitivity_  { nullptr };
		juce::AudioParameterFloat*  pMix_          { nullptr };
		juce::AudioParameterFloat*  pOutput_       { nullptr };
		juce::AudioParameterChoice* pQuality_      { nullptr };
		juce::AudioParameterFloat*  pGuard_        { nullptr };
		juce::AudioParameterChoice* pActivityScale_{ nullptr };
		juce::AudioParameterFloat*  pHeadroom_     { nullptr };

		QualityMode lastMode_ { QualityMode::Standard };

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
	};
}
