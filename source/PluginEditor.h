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

    VoxMidiAudioProcessor& processor;
    VoxLookAndFeel lnf;

    juce::ComboBox presetBox, keyBox, scaleBox, responseBox;
    juce::Slider   sensitivity, smoothing, octave;
    juce::Label    suggestText;
    juce::TextButton useKeyBtn { "Use key" }, usePresetBtn { "Use preset" };

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SA> sensAtt, smoothAtt, octaveAtt;
    std::unique_ptr<CA> keyAtt, scaleAtt, respAtt;

    // layout rectangles shared with paint()
    juce::Rectangle<int> readoutPanel, readoutBounds, meterBounds, suggestPanel;

    // live readout state
    int   note = -1;
    float freq = 0.0f;
    float level = 0.0f;

    // suggestion state
    int sugKey = -1, sugScale = 1, sugPreset = -1;
    int ticks = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxMidiAudioProcessorEditor)
};
