#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>

#include "dsp/YinPitchDetector.h"
#include "dsp/NoteTracker.h"
#include "dsp/KeyEstimator.h"

// VoxMIDI — voice in -> MIDI notes out. Parameters live in an APVTS (presets, save/load,
// host automation). The signal path: detect pitch (YIN) -> stable note (NoteTracker) ->
// octave shift + key/scale snap -> MIDI (host buffer for VST3, loopMIDI for standalone).
// A KeyEstimator listens to what you sing and suggests a key + preset.
class VoxMidiAudioProcessor : public juce::AudioProcessor
{
public:
    VoxMidiAudioProcessor();
    ~VoxMidiAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
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

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- GUI-facing state ----
    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> currentLevel { 0.0f }; // input peak 0..1
    std::atomic<int>   displayNote  { -1 };   // note being output (post octave + snap)
    std::atomic<int>   rawNote      { -1 };   // detected note pre-snap (feeds key estimate)
    std::atomic<float> currentFreq  { 0.0f };

    // recommendation — called from the editor timer (message thread only)
    void pushForRecommendation();
    vox::KeyEstimator::Result getRecommendation() const;

    // UI actions
    void setParamValue (const juce::String& id, float value);
    void applyPreset (int presetIndex);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    YinPitchDetector  detector;
    NoteTracker       tracker;
    vox::KeyEstimator keyEstimator;

    std::vector<float> ring;      // circular buffer of recent mono samples
    std::vector<float> analysis;  // linearized window handed to the detector
    int ringSize = 4096;          // == max analysis window
    int hopSize = 512;
    int ringWrite = 0;
    int sinceAnalysis = 0;

    std::unique_ptr<juce::MidiOutput> midiOut; // standalone -> loopMIDI
    int lastEmittedNote = -1;

    std::atomic<float>* pSensitivity = nullptr;
    std::atomic<float>* pSmoothing   = nullptr;
    std::atomic<float>* pResponse    = nullptr;
    std::atomic<float>* pOctave      = nullptr;
    std::atomic<float>* pKey         = nullptr;
    std::atomic<float>* pScale       = nullptr;
    std::atomic<float>* pChannel     = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxMidiAudioProcessor)
};
