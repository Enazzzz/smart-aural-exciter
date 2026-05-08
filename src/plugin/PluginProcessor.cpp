#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace smex
{
	PluginProcessor::PluginProcessor()
		: AudioProcessor (BusesProperties()
		                  .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
		                  .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
		  apvts_ (*this, nullptr, "SmartExciter", createParameterLayout())
	{
		// Resolve raw parameter pointers once. APVTS owns the lifetime; we
		// only need access for cheap per-block reads.
		pAmount_        = dynamic_cast<juce::AudioParameterFloat*> (apvts_.getParameter (ParamId::amount));
		pTone_          = dynamic_cast<juce::AudioParameterFloat*> (apvts_.getParameter (ParamId::tone));
		pSensitivity_   = dynamic_cast<juce::AudioParameterFloat*> (apvts_.getParameter (ParamId::sensitivity));
		pMix_           = dynamic_cast<juce::AudioParameterFloat*> (apvts_.getParameter (ParamId::mix));
		pOutput_        = dynamic_cast<juce::AudioParameterFloat*> (apvts_.getParameter (ParamId::output));
		pQuality_       = dynamic_cast<juce::AudioParameterChoice*>(apvts_.getParameter (ParamId::qualityMode));
		pGuard_         = dynamic_cast<juce::AudioParameterFloat*> (apvts_.getParameter (ParamId::guardAmount));
		pActivityScale_ = dynamic_cast<juce::AudioParameterChoice*>(apvts_.getParameter (ParamId::activityScale));
		pHeadroom_      = dynamic_cast<juce::AudioParameterFloat*> (apvts_.getParameter (ParamId::headroomTrim));
	}

	bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
	{
		// V1 ships stereo only - reject mono and surround at the host level.
		const auto in  = layouts.getMainInputChannelSet();
		const auto out = layouts.getMainOutputChannelSet();
		return in == juce::AudioChannelSet::stereo()
		    && out == juce::AudioChannelSet::stereo();
	}

	void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
	{
		engine_.prepare (sampleRate, samplesPerBlock, 2);
		setLatencySamples (engine_.getLatencySamples());
		lastMode_ = QualityMode::Standard;
	}

	void PluginProcessor::releaseResources()
	{
		// All buffers are RAII; nothing to release explicitly. Keeping the
		// override so JUCE's lifecycle model stays explicit.
	}

	void PluginProcessor::pushParamsIntoEngine()
	{
		DspEngine::LiveParams p;
		p.amount       = pAmount_       ? pAmount_->get()       * 0.01f : 0.35f;
		// Tone is -50..+50 in the spec; engine wants -1..+1.
		p.tone         = pTone_         ? pTone_->get()         * 0.02f : 0.0f;
		p.sensitivity  = pSensitivity_  ? pSensitivity_->get()  * 0.01f : 0.5f;
		p.mix          = pMix_          ? pMix_->get()          * 0.01f : 1.0f;
		p.outputGain   = pOutput_       ? juce::Decibels::decibelsToGain (pOutput_->get())   : 1.0f;
		p.guardAmount  = pGuard_        ? pGuard_->get()        * 0.01f : 0.6f;
		p.headroomGain = pHeadroom_     ? juce::Decibels::decibelsToGain (pHeadroom_->get()) : 0.7079f;
		engine_.setParams (p);

		// Quality mode change triggers a host PDC update.
		const QualityMode mode = pQuality_ && pQuality_->getIndex() == static_cast<int>(QualityMode::High)
			? QualityMode::High
			: QualityMode::Standard;
		if (mode != lastMode_)
		{
			engine_.setQualityMode (mode);
			setLatencySamples (engine_.getLatencySamples());
			lastMode_ = mode;
		}
	}

	void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
	{
		const juce::ScopedNoDenormals noDenormals;

		// Defensive: clear any extra output channels we don't process.
		const auto totalNumIn  = getTotalNumInputChannels();
		const auto totalNumOut = getTotalNumOutputChannels();
		for (int ch = totalNumIn; ch < totalNumOut; ++ch)
			buffer.clear (ch, 0, buffer.getNumSamples());

		pushParamsIntoEngine();

		const bool internallyBypassed = false;
		engine_.process (buffer, internallyBypassed);
	}

	juce::AudioProcessorEditor* PluginProcessor::createEditor()
	{
		return new PluginEditor (*this);
	}

	void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
	{
		auto state = apvts_.copyState();
		std::unique_ptr<juce::XmlElement> xml (state.createXml());
		copyXmlToBinary (*xml, destData);
	}

	void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
	{
		std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
		if (xml != nullptr && xml->hasTagName (apvts_.state.getType()))
			apvts_.replaceState (juce::ValueTree::fromXml (*xml));
	}
}

// ---------------------------------------------------------------------------
// JUCE plugin factory
// ---------------------------------------------------------------------------
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new smex::PluginProcessor();
}
