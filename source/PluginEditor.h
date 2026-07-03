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

    juce::ComboBox presetBox, modeBox, keyBox, scaleBox, responseBox, chordBox, patternBox, rateBox,
                   modSourceBox, velModeBox;
    juce::Slider   sensitivity, smoothing, drumSens, octave, arpOctaves, arpGate, transpose, tempo,
                   bendRange, glide, modCC, modAmount, velFixed, velCurve;
    juce::TextButton holdBtn { "Hold" }, arpBtn { "Arp" }, octMinusBtn { "Oct -" }, octPlusBtn { "Oct +" },
                     bendBtn { "Bend" };
    juce::Label    suggestText;
    juce::TextButton useKeyBtn { "Use key" }, usePresetBtn { "Use preset" };

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SA> sensAtt, smoothAtt, drumSensAtt, octaveAtt, arpOctAtt, arpGateAtt, transposeAtt, tempoAtt,
                        bendRangeAtt, glideAtt, modCCAtt, modAmountAtt, velFixedAtt, velCurveAtt;
    std::unique_ptr<CA> modeAtt, keyAtt, scaleAtt, respAtt, chordAtt, patternAtt, rateAtt, modSourceAtt, velModeAtt;
    std::unique_ptr<BA> holdAtt, arpAtt, bendAtt;

    juce::Rectangle<int> readoutPanel, readoutBounds, meterBounds, suggestPanel, inputHeader, perfHeader, exprHeader;

    int   note = -1;
    float freq = 0.0f;
    float level = 0.0f;

    int sugKey = -1, sugScale = 1, sugPreset = -1;
    int ticks = 0;

    int lastSeenHit = 0;
    int drumFlashTicks = 0;
    int drumNote = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxMidiAudioProcessorEditor)
};
