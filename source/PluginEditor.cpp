#include "PluginEditor.h"

VoxMidiAudioProcessorEditor::VoxMidiAudioProcessorEditor (VoxMidiAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (360, 260);
    startTimerHz (30);
}

VoxMidiAudioProcessorEditor::~VoxMidiAudioProcessorEditor()
{
    stopTimer();
}

juce::String VoxMidiAudioProcessorEditor::midiNoteName (int midi)
{
    if (midi < 0)
        return "--";
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F",
                                   "F#", "G", "G#", "A", "A#", "B" };
    return juce::String (names[midi % 12]) + juce::String (midi / 12 - 1);
}

void VoxMidiAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e1e));

    auto area = getLocalBounds();

    g.setColour (juce::Colours::white);
    g.setFont (20.0f);
    g.drawText ("VoxMIDI", area.removeFromTop (36), juce::Justification::centred);

    g.setColour (juce::Colours::grey);
    g.setFont (11.0f);
    g.drawText ("M2 - live pitch detection (no MIDI out yet)",
                area.removeFromTop (18), juce::Justification::centred);

    // big detected-note readout
    const bool active = (note >= 0);
    g.setColour (active ? juce::Colours::aqua : juce::Colours::dimgrey);
    g.setFont (juce::Font (64.0f, juce::Font::bold));
    g.drawText (midiNoteName (note), area.removeFromTop (96),
                juce::Justification::centred);

    g.setColour (juce::Colours::grey);
    g.setFont (14.0f);
    g.drawText (active ? (juce::String (freq, 1) + " Hz") : juce::String ("listening..."),
                area.removeFromTop (24), juce::Justification::centred);

    // input meter along the bottom
    auto meter = getLocalBounds().reduced (30).removeFromBottom (28);
    g.setColour (juce::Colours::grey);
    g.setFont (10.0f);
    g.drawText ("input", meter.translated (0, -16), juce::Justification::centredLeft);
    g.setColour (juce::Colours::darkgrey);
    g.drawRect (meter, 1);
    g.setColour (juce::Colours::limegreen);
    auto fill = meter.toFloat().reduced (1.0f);
    fill = fill.withWidth (fill.getWidth() * juce::jlimit (0.0f, 1.0f, level));
    g.fillRect (fill);
}

void VoxMidiAudioProcessorEditor::resized() {}

void VoxMidiAudioProcessorEditor::timerCallback()
{
    bool changed = false;

    const float l = processor.currentLevel.load();
    const int   nt = processor.currentNote.load();
    const float f = processor.currentFreq.load();

    if (std::abs (l - level) > 0.001f) { level = l; changed = true; }
    if (nt != note)                    { note  = nt; changed = true; }
    if (std::abs (f - freq) > 0.5f)    { freq  = f; changed = true; }

    if (changed)
        repaint();
}
