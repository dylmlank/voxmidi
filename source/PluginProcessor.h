#pragma once

#include <JuceHeader.h>
#include <vector>

#include "dsp/YinPitchDetector.h"
#include "dsp/NoteTracker.h"

// VoxMIDI — v1 target: audio (voice) in -> MIDI notes out.
// M2 (this file): live monophonic pitch detection. The detected note is computed on
// the audio thread and shown in the editor. MIDI output is still M3; scale-snapping M4.
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

    // Shared with the editor (audio thread writes, GUI thread reads — lock-free).
    std::atomic<float> currentLevel { 0.0f };  // input peak 0..1
    std::atomic<int>   currentNote  { -1 };    // detected MIDI note, -1 = none
    std::atomic<float> currentFreq  { 0.0f };  // detected frequency in Hz

private:
    YinPitchDetector detector;
    NoteTracker      tracker;

    std::vector<float> ring;      // circular buffer of recent mono samples (size window)
    std::vector<float> analysis;  // linearized window handed to the detector
    int windowSize = 2048;        // ~46 ms @ 44.1k — bigger = better low notes, more lag
    int hopSize = 512;            // analyse this often (~11.6 ms @ 44.1k)
    int ringWrite = 0;
    int sinceAnalysis = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxMidiAudioProcessor)
};
