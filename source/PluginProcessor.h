#pragma once

#include <JuceHeader.h>

// VoxMIDI — v1 target: audio (voice) in -> MIDI notes out.
// M1 (this file): a working plugin shell that loads in FL and a standalone,
// passes audio through, and reports input level. Voice detection + MIDI come next.
class VoxMidiAudioProcessor : public juce::AudioProcessor
{
public:
    VoxMidiAudioProcessor();
    ~VoxMidiAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VoxMIDI"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    // Live input level (0..1), read by the editor for the meter. Audio thread writes,
    // GUI thread reads — atomic keeps it lock-free.
    std::atomic<float> currentLevel { 0.0f };

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxMidiAudioProcessor)
};
