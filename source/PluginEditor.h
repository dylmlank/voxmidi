#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class VoxMidiAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit VoxMidiAudioProcessorEditor (VoxMidiAudioProcessor&);
    ~VoxMidiAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    VoxMidiAudioProcessor& processor;
    float level = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxMidiAudioProcessorEditor)
};
