#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "VoxLookAndFeel.h"

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
    static juce::String noteName (int midi);
    static juce::String drumName (int gmNote);

    VoxMidiAudioProcessor& processor;
    VoxLookAndFeel lnf;

    juce::ComboBox presetBox, modeBox, keyBox, scaleBox, responseBox;
    juce::Slider   sensitivity, smoothing, octave, drumSens;
    juce::Label    suggestText;
    juce::TextButton useKeyBtn { "Use key" }, usePresetBtn { "Use preset" };

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SA> sensAtt, smoothAtt, octaveAtt, drumSensAtt;
    std::unique_ptr<CA> modeAtt, keyAtt, scaleAtt, respAtt;

    juce::Rectangle<int> readoutPanel, readoutBounds, meterBounds, suggestPanel;

    int   note = -1;
    float freq = 0.0f;
    float level = 0.0f;

    int sugKey = -1, sugScale = 1, sugPreset = -1;
    int ticks = 0;

    // drum-hit flash
    int lastSeenHit = 0;
    int drumFlashTicks = 0;
    int drumNote = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxMidiAudioProcessorEditor)
};
