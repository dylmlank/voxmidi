#include "PluginEditor.h"
#include "Presets.h"

#include <cmath>

namespace
{
    const char* kKeyNames[] = { "C", "C#", "D", "D#", "E", "F",
                                "F#", "G", "G#", "A", "A#", "B" };
}

VoxMidiAudioProcessorEditor::VoxMidiAudioProcessorEditor (VoxMidiAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lnf);

    presetBox.setTextWhenNothingSelected ("Presets");
    for (int i = 0; i < vox::NUM_PRESETS; ++i)
        presetBox.addItem (vox::PRESETS[i].name, i + 1);
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id > 0) processor.applyPreset (id - 1);
    };
    addAndMakeVisible (presetBox);

    keyBox.addItemList ({ "C", "C#", "D", "D#", "E", "F",
                          "F#", "G", "G#", "A", "A#", "B" }, 1);
    scaleBox.addItemList ({ "Chromatic", "Major", "Minor", "Dorian", "Mixolydian",
                            "Pentatonic Maj", "Pentatonic Min", "Blues", "Harmonic Min" }, 1);
    responseBox.addItemList ({ "Fast", "Balanced", "Accurate" }, 1);
    addAndMakeVisible (keyBox);
    addAndMakeVisible (scaleBox);
    addAndMakeVisible (responseBox);
    keyAtt   = std::make_unique<CA> (processor.apvts, "key", keyBox);
    scaleAtt = std::make_unique<CA> (processor.apvts, "scale", scaleBox);
    respAtt  = std::make_unique<CA> (processor.apvts, "response", responseBox);

    auto initKnob = [this] (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
        addAndMakeVisible (s);
    };
    initKnob (sensitivity);
    initKnob (smoothing);
    initKnob (octave);
    sensAtt   = std::make_unique<SA> (processor.apvts, "sensitivity", sensitivity);
    smoothAtt = std::make_unique<SA> (processor.apvts, "smoothing", smoothing);
    octaveAtt = std::make_unique<SA> (processor.apvts, "octave", octave);

    suggestText.setText ("Listening...", juce::dontSendNotification);
    suggestText.setColour (juce::Label::textColourId, VoxLookAndFeel::text);
    suggestText.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (suggestText);

    useKeyBtn.setEnabled (false);
    usePresetBtn.setEnabled (false);
    useKeyBtn.onClick = [this]
    {
        if (sugKey >= 0)
        {
            processor.setParamValue ("key", (float) sugKey);
            processor.setParamValue ("scale", (float) sugScale);
        }
    };
    usePresetBtn.onClick = [this]
    {
        if (sugPreset >= 0)
        {
            processor.applyPreset (sugPreset);
            presetBox.setSelectedId (sugPreset + 1, juce::dontSendNotification);
        }
    };
    addAndMakeVisible (useKeyBtn);
    addAndMakeVisible (usePresetBtn);

    setSize (560, 470);
    startTimerHz (30);
}

VoxMidiAudioProcessorEditor::~VoxMidiAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

juce::String VoxMidiAudioProcessorEditor::noteName (int midi)
{
    if (midi < 0)
        return "--";
    return juce::String (kKeyNames[midi % 12]) + juce::String (midi / 12 - 1);
}

void VoxMidiAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (16);

    auto header = r.removeFromTop (40);
    presetBox.setBounds (header.removeFromRight (170).withSizeKeepingCentre (170, 30));

    r.removeFromTop (12);

    auto mid = r.removeFromTop (104);
    readoutPanel = mid.removeFromLeft (268);
    {
        auto rp = readoutPanel;
        meterBounds = rp.removeFromBottom (18).reduced (10, 5);
        readoutBounds = rp;
    }
    mid.removeFromLeft (16);
    suggestPanel = mid;
    {
        auto sp = suggestPanel.reduced (12, 10);
        sp.removeFromTop (18); // caption drawn in paint
        suggestText.setBounds (sp.removeFromTop (26));
        sp.removeFromTop (6);
        auto btns = sp.removeFromTop (28);
        useKeyBtn.setBounds (btns.removeFromLeft (84));
        btns.removeFromLeft (8);
        usePresetBtn.setBounds (btns.removeFromLeft (96));
    }

    r.removeFromTop (26);
    auto combos = r.removeFromTop (44);
    keyBox.setBounds (combos.removeFromLeft (104).withSizeKeepingCentre (104, 28));
    combos.removeFromLeft (14);
    scaleBox.setBounds (combos.removeFromLeft (150).withSizeKeepingCentre (150, 28));
    combos.removeFromLeft (14);
    responseBox.setBounds (combos.removeFromLeft (120).withSizeKeepingCentre (120, 28));

    r.removeFromTop (30);
    const int kw = r.getWidth() / 3;
    sensitivity.setBounds (r.removeFromLeft (kw).reduced (8));
    smoothing.setBounds (r.removeFromLeft (kw).reduced (8));
    octave.setBounds (r.removeFromLeft (kw).reduced (8));
}

void VoxMidiAudioProcessorEditor::paint (juce::Graphics& g)
{
    using L = VoxLookAndFeel;
    g.fillAll (L::bg);

    // title
    g.setColour (L::text);
    g.setFont (juce::Font (22.0f, juce::Font::bold));
    g.drawText ("VoxMIDI", 16, 12, 240, 26, juce::Justification::centredLeft);
    g.setColour (L::dim);
    g.setFont (11.0f);
    g.drawText ("voice → MIDI", 16, 34, 240, 14, juce::Justification::centredLeft);

    // readout panel
    g.setColour (L::panel);
    g.fillRoundedRectangle (readoutPanel.toFloat(), 8.0f);
    const bool active = (note >= 0);
    {
        auto rb = readoutBounds.reduced (10);
        auto hz = rb.removeFromBottom (22);
        g.setColour (active ? L::accent : L::dim);
        g.setFont (juce::Font (52.0f, juce::Font::bold));
        g.drawText (noteName (note), rb, juce::Justification::centred);
        g.setColour (L::dim);
        g.setFont (13.0f);
        g.drawText (active ? (juce::String (freq, 1) + " Hz") : juce::String ("listening..."),
                    hz, juce::Justification::centred);
    }
    // input meter
    g.setColour (L::bg.brighter (0.06f));
    g.fillRoundedRectangle (meterBounds.toFloat(), 3.0f);
    g.setColour (L::accent);
    auto fill = meterBounds.toFloat().reduced (1.0f);
    fill = fill.withWidth (fill.getWidth() * juce::jlimit (0.0f, 1.0f, level));
    g.fillRoundedRectangle (fill, 3.0f);

    // suggestion panel
    g.setColour (L::panel);
    g.fillRoundedRectangle (suggestPanel.toFloat(), 8.0f);
    g.setColour (L::dim);
    g.setFont (11.0f);
    g.drawText ("SUGGESTED", suggestPanel.getX() + 12, suggestPanel.getY() + 10,
                200, 14, juce::Justification::centredLeft);

    // control captions
    auto caption = [&] (juce::Component& c, const juce::String& t, bool below)
    {
        auto b = c.getBounds();
        g.setColour (L::dim);
        g.setFont (11.0f);
        const int cy = below ? b.getBottom() + 2 : b.getY() - 16;
        g.drawText (t, b.getX(), cy, b.getWidth(), 14, juce::Justification::centred);
    };
    caption (keyBox, "KEY", false);
    caption (scaleBox, "SCALE", false);
    caption (responseBox, "RESPONSE", false);
    caption (sensitivity, "SENSITIVITY", false);
    caption (smoothing, "SMOOTHING", false);
    caption (octave, "OCTAVE", false);
}

void VoxMidiAudioProcessorEditor::timerCallback()
{
    bool changed = false;

    const float l  = processor.currentLevel.load();
    const int   nt = processor.displayNote.load();
    const float f  = processor.currentFreq.load();
    if (std::abs (l - level) > 0.005f) { level = l; changed = true; }
    if (nt != note)                    { note = nt; changed = true; }
    if (std::abs (f - freq) > 0.5f)    { freq = f; changed = true; }

    processor.pushForRecommendation();
    if (++ticks % 15 == 0)
    {
        const auto rec = processor.getRecommendation();
        if (rec.valid)
        {
            sugKey = rec.key;
            sugScale = rec.scaleIndex;
            sugPreset = rec.presetIndex;

            const juce::String scaleName = (rec.scaleIndex == 2) ? "Minor" : "Major";
            suggestText.setText (juce::String (kKeyNames[rec.key]) + " " + scaleName
                                     + "  ·  " + vox::PRESETS[rec.presetIndex].name,
                                 juce::dontSendNotification);
            useKeyBtn.setEnabled (true);
            usePresetBtn.setEnabled (true);
        }
    }

    if (changed)
        repaint();
}
