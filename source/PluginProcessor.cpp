#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"
#include "dsp/ScaleSnapper.h"

#include <cmath>

juce::AudioProcessorValueTreeState::ParameterLayout VoxMidiAudioProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    const StringArray keys   { "C", "C#", "D", "D#", "E", "F",
                               "F#", "G", "G#", "A", "A#", "B" };
    const StringArray scales { "Chromatic", "Major", "Minor", "Dorian", "Mixolydian",
                               "Pentatonic Maj", "Pentatonic Min", "Blues", "Harmonic Min" };
    const StringArray resp   { "Fast", "Balanced", "Accurate" };

    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { "key", 1 },   "Key",   keys,   0));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { "scale", 1 }, "Scale", scales, 1));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "sensitivity", 1 }, "Sensitivity",
                                                       NormalisableRange<float> (0.0f, 100.0f, 1.0f), 55.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "smoothing", 1 }, "Smoothing",
                                                       NormalisableRange<float> (0.0f, 100.0f, 1.0f), 40.0f));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { "response", 1 }, "Response", resp, 1));
    layout.add (std::make_unique<AudioParameterInt>   (ParameterID { "octave", 1 },  "Octave",  -2, 2, 0));
    layout.add (std::make_unique<AudioParameterInt>   (ParameterID { "channel", 1 }, "MIDI Ch", 1, 16, 1));
    return layout;
}

VoxMidiAudioProcessor::VoxMidiAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pSensitivity = apvts.getRawParameterValue ("sensitivity");
    pSmoothing   = apvts.getRawParameterValue ("smoothing");
    pResponse    = apvts.getRawParameterValue ("response");
    pOctave      = apvts.getRawParameterValue ("octave");
    pKey         = apvts.getRawParameterValue ("key");
    pScale       = apvts.getRawParameterValue ("scale");
    pChannel     = apvts.getRawParameterValue ("channel");
}

void VoxMidiAudioProcessor::prepareToPlay (double sampleRate, int)
{
    ringSize = 4096;
    hopSize = 512;

    detector.setup (sampleRate, ringSize);
    tracker.setup (sampleRate);

    ring.assign ((size_t) ringSize, 0.0f);
    analysis.assign ((size_t) ringSize, 0.0f);
    ringWrite = 0;
    sinceAnalysis = 0;
    lastEmittedNote = -1;

    if (wrapperType == wrapperType_Standalone && midiOut == nullptr)
    {
        for (auto& d : juce::MidiOutput::getAvailableDevices())
        {
            if (d.name.containsIgnoreCase ("loopMIDI"))
            {
                midiOut = juce::MidiOutput::openDevice (d.identifier);
                break;
            }
        }
    }
}

void VoxMidiAudioProcessor::releaseResources()
{
    if (midiOut != nullptr && lastEmittedNote >= 0)
        midiOut->sendMessageNow (juce::MidiMessage::noteOff (1, lastEmittedNote));
    lastEmittedNote = -1;
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
    midi.clear();

    const int n  = buffer.getNumSamples();
    const int ch = juce::jmax (1, buffer.getNumChannels());

    // read parameters once per block
    const float sens     = pSensitivity->load();
    const float smooth   = pSmoothing->load();
    const int   respIdx  = (int) pResponse->load();
    const int   octave   = (int) pOctave->load();
    const int   key      = (int) pKey->load();
    const int   scaleIdx = (int) pScale->load();
    const int   channel  = juce::jlimit (1, 16, (int) pChannel->load());

    tracker.setLevelGate      (juce::jmap (sens, 0.0f, 100.0f, 0.06f, 0.004f));
    tracker.setConfidenceGate (juce::jmap (sens, 0.0f, 100.0f, 0.97f, 0.80f));
    tracker.setHysteresisFrames (juce::jlimit (1, 8, juce::roundToInt (juce::jmap (smooth, 0.0f, 100.0f, 1.0f, 8.0f))));

    const int activeWindow = (respIdx == 0) ? 1024 : (respIdx == 2) ? 4096 : 2048;
    const bool toLoop = (wrapperType == wrapperType_Standalone && midiOut != nullptr);

    float peak = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        float s = 0.0f;
        for (int c = 0; c < buffer.getNumChannels(); ++c)
            s += buffer.getSample (c, i);
        s /= (float) ch;

        peak = juce::jmax (peak, std::abs (s));

        ring[(size_t) ringWrite] = s;
        ringWrite = (ringWrite + 1) % ringSize;

        if (++sinceAnalysis >= hopSize)
        {
            sinceAnalysis = 0;

            const int start = ((ringWrite - activeWindow) % ringSize + ringSize) % ringSize;
            float sq = 0.0f;
            for (int k = 0; k < activeWindow; ++k)
            {
                const float v = ring[(size_t) ((start + k) % ringSize)];
                analysis[(size_t) k] = v;
                sq += v * v;
            }
            const float rms = std::sqrt (sq / (float) activeWindow);

            float prob = 0.0f;
            const float freq = detector.process (analysis.data(), activeWindow, prob);
            const int raw = tracker.process (freq, prob, rms);
            rawNote.store (raw);

            int out = raw;
            if (out >= 0)
            {
                out = vox::snapToScale (out, key, scaleIdx) + 12 * octave;
                out = juce::jlimit (0, 127, out);
            }
            displayNote.store (out);
            currentFreq.store (tracker.getCurrentFrequency());

            if (out != lastEmittedNote)
            {
                const int vel = juce::jlimit (1, 127,
                    juce::roundToInt (juce::jmap (rms, 0.0f, 0.2f, 40.0f, 127.0f)));

                if (lastEmittedNote >= 0)
                {
                    const auto off = juce::MidiMessage::noteOff (channel, lastEmittedNote);
                    midi.addEvent (off, i);
                    if (toLoop) midiOut->sendMessageNow (off);
                }
                if (out >= 0)
                {
                    const auto on = juce::MidiMessage::noteOn (channel, out, (juce::uint8) vel);
                    midi.addEvent (on, i);
                    if (toLoop) midiOut->sendMessageNow (on);
                }
                lastEmittedNote = out;
            }
        }
    }

    currentLevel.store (peak);
}

void VoxMidiAudioProcessor::pushForRecommendation()
{
    keyEstimator.addSample (rawNote.load());
}

vox::KeyEstimator::Result VoxMidiAudioProcessor::getRecommendation() const
{
    return keyEstimator.estimate();
}

void VoxMidiAudioProcessor::setParamValue (const juce::String& id, float value)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void VoxMidiAudioProcessor::applyPreset (int presetIndex)
{
    if (presetIndex < 0 || presetIndex >= vox::NUM_PRESETS)
        return;
    const auto& pr = vox::PRESETS[presetIndex];
    setParamValue ("sensitivity", pr.sensitivity);
    setParamValue ("smoothing",   pr.smoothing);
    setParamValue ("response",    (float) pr.response);
    setParamValue ("octave",      (float) pr.octave);
}

void VoxMidiAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = std::unique_ptr<juce::XmlElement> (apvts.copyState().createXml()))
        copyXmlToBinary (*xml, dest);
}

void VoxMidiAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = std::unique_ptr<juce::XmlElement> (getXmlFromBinary (data, size)))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* VoxMidiAudioProcessor::createEditor()
{
    return new VoxMidiAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoxMidiAudioProcessor();
}
