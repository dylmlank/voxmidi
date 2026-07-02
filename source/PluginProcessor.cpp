#include "PluginProcessor.h"
#include "PluginEditor.h"

VoxMidiAudioProcessor::VoxMidiAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void VoxMidiAudioProcessor::prepareToPlay (double, int) {}

bool VoxMidiAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    // input must match output channel count (mono voice is the usual case)
    return layouts.getMainInputChannelSet() == out;
}

void VoxMidiAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    // M1: measure peak input for the meter, pass audio through untouched, emit no MIDI.
    // M2+ will feed the buffer to the pitch detector and fill `midi` with note events.
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        peak = juce::jmax (peak, buffer.getMagnitude (ch, 0, buffer.getNumSamples()));
    currentLevel.store (peak);

    juce::ignoreUnused (midi); // MIDI output wired in M3
}

juce::AudioProcessorEditor* VoxMidiAudioProcessor::createEditor()
{
    return new VoxMidiAudioProcessorEditor (*this);
}

// Entry point JUCE calls to create the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoxMidiAudioProcessor();
}
