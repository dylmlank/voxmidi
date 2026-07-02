#include "PluginEditor.h"

VoxMidiAudioProcessorEditor::VoxMidiAudioProcessorEditor (VoxMidiAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (360, 200);
    startTimerHz (30);
}

VoxMidiAudioProcessorEditor::~VoxMidiAudioProcessorEditor()
{
    stopTimer();
}

void VoxMidiAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e1e));

    g.setColour (juce::Colours::white);
    g.setFont (22.0f);
    g.drawText ("VoxMIDI", getLocalBounds().removeFromTop (44),
                juce::Justification::centred);

    g.setColour (juce::Colours::grey);
    g.setFont (12.0f);
    g.drawText ("v0.1 — shell only (M1): passes audio, no MIDI yet",
                getLocalBounds().withTrimmedTop (46).removeFromTop (22),
                juce::Justification::centred);

    // input meter
    auto meter = getLocalBounds().reduced (30).removeFromBottom (34);
    g.setColour (juce::Colours::darkgrey);
    g.drawRect (meter, 1);
    g.setColour (juce::Colours::limegreen);
    auto fill = meter.toFloat().reduced (1.0f);
    fill = fill.withWidth (fill.getWidth() * juce::jlimit (0.0f, 1.0f, level));
    g.fillRect (fill);

    g.setColour (juce::Colours::grey);
    g.setFont (11.0f);
    g.drawText ("input level", meter.translated (0, -18),
                juce::Justification::centredLeft);
}

void VoxMidiAudioProcessorEditor::resized() {}

void VoxMidiAudioProcessorEditor::timerCallback()
{
    const float l = processor.currentLevel.load();
    if (std::abs (l - level) > 0.001f)
    {
        level = l;
        repaint();
    }
}
