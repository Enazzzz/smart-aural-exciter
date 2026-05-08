#include "PluginEditor.h"

namespace smex
{
	PluginEditor::PluginEditor (PluginProcessor& p)
		: AudioProcessorEditor (&p),
		  processor_ (p),
		  simple_   (p.getApvts()),
		  advanced_ (p.getApvts())
	{
		addAndMakeVisible (simple_);
		addAndMakeVisible (advanced_);
		addAndMakeVisible (meters_);
		addAndMakeVisible (activity_);
		addAndMakeVisible (advancedToggle_);

		advancedToggle_.onClick = [this]
		{
			advanced_.setVisible (advancedToggle_.getToggleState());
			resized();
		};
		advanced_.setVisible (false);

		setSize (640, 360);

		// 30 Hz UI refresh - matches the activity view's spec'd refresh rate.
		startTimerHz (30);
	}

	void PluginEditor::paint (juce::Graphics& g)
	{
		g.fillAll (juce::Colours::black.withAlpha (0.85f));

		g.setColour (juce::Colours::white.withAlpha (0.7f));
		g.setFont (16.0f);
		g.drawText ("Smart Exciter", getLocalBounds().reduced (12, 8),
		            juce::Justification::topLeft);
	}

	void PluginEditor::resized()
	{
		auto r = getLocalBounds().reduced (12);

		auto top = r.removeFromTop (28);
		advancedToggle_.setBounds (top.removeFromRight (110));

		// Simple panel sits across the top, meters next to it, activity below.
		auto simpleArea = r.removeFromTop (130);
		simple_.setBounds (simpleArea.removeFromLeft (simpleArea.getWidth() - 200));
		meters_.setBounds (simpleArea.reduced (8, 0));

		auto rest = r;
		if (advanced_.isVisible())
		{
			auto advArea = rest.removeFromTop (90);
			advanced_.setBounds (advArea);
		}

		activity_.setBounds (rest);
	}

	void PluginEditor::timerCallback()
	{
		meters_.setSnapshot   (processor_.getEngine().meterSnapshot());
		activity_.setSnapshot (processor_.getEngine().activitySnapshot(),
		                        getActivityScale (processor_.getApvts()));
		meters_.repaint();
		activity_.repaint();
	}
}
