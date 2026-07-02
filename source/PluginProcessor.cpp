#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

VoxMidiAudioProcessor::VoxMidiAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void VoxMidiAudioProcessor::prepareToPlay (double sampleRate, int)
{
    windowSize = 2048;
    hopSize = 512;

    detector.setup (sampleRate, windowSize);
    tracker.setup (sampleRate);

    ring.assign ((size_t) windowSize, 0.0f);
    analysis.assign ((size_t) windowSize, 0.0f);
    ringWrite = 0;
    sinceAnalysis = 0;
}

bool VoxMidiAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void VoxMidiAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int n  = buffer.getNumSamples();
    const int ch = juce::jmax (1, buffer.getNumChannels());

    float peak = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        // downmix to mono
        float s = 0.0f;
        for (int c = 0; c < buffer.getNumChannels(); ++c)
            s += buffer.getSample (c, i);
        s /= (float) ch;

        peak = juce::jmax (peak, std::abs (s));

        ring[(size_t) ringWrite] = s;
        ringWrite = (ringWrite + 1) % windowSize;

        if (++sinceAnalysis >= hopSize)
        {
            sinceAnalysis = 0;

            // linearize the ring buffer oldest -> newest
            float sq = 0.0f;
            for (int k = 0; k < windowSize; ++k)
            {
                const float v = ring[(size_t) ((ringWrite + k) % windowSize)];
                analysis[(size_t) k] = v;
                sq += v * v;
            }
            const float rms = std::sqrt (sq / (float) windowSize);

            float prob = 0.0f;
            const float freq = detector.process (analysis.data(), windowSize, prob);
            const int note = tracker.process (freq, prob, rms);

            currentNote.store (note);
            currentFreq.store (tracker.getCurrentFrequency());
        }
    }

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
