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

    modeBox.addItemList ({ "Melody", "Drums", "Both" }, 1);
    keyBox.addItemList ({ "C", "C#", "D", "D#", "E", "F",
                          "F#", "G", "G#", "A", "A#", "B" }, 1);
    scaleBox.addItemList ({ "Chromatic", "Major", "Minor", "Dorian", "Mixolydian",
                            "Pentatonic Maj", "Pentatonic Min", "Blues", "Harmonic Min" }, 1);
    responseBox.addItemList ({ "Fast", "Balanced", "Accurate" }, 1);
    addAndMakeVisible (modeBox);
    addAndMakeVisible (keyBox);
    addAndMakeVisible (scaleBox);
    addAndMakeVisible (responseBox);
    modeAtt  = std::make_unique<CA> (processor.apvts, "mode", modeBox);
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
    initKnob (drumSens);
    sensAtt     = std::make_unique<SA> (processor.apvts, "sensitivity", sensitivity);
    smoothAtt   = std::make_unique<SA> (processor.apvts, "smoothing", smoothing);
    octaveAtt   = std::make_unique<SA> (processor.apvts, "octave", octave);
    drumSensAtt = std::make_unique<SA> (processor.apvts, "drumSensitivity", drumSens);

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

    setSize (640, 500);
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

juce::String VoxMidiAudioProcessorEditor::drumName (int gmNote)
{
    switch (gmNote)
    {
        case 36: return "KICK";
        case 38: return "SNARE";
        case 42: return "HI-HAT";
        default: return "HIT";
    }
}

void VoxMidiAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (16);

    auto header = r.removeFromTop (40);
    presetBox.setBounds (header.removeFromRight (170).withSizeKeepingCentre (170, 30));

    r.removeFromTop (12);

    auto mid = r.removeFromTop (104);
    readoutPanel = mid.removeFromLeft (280);
    {
        auto rp = readoutPanel;
        meterBounds = rp.removeFromBottom (18).reduced (10, 5);
        readoutBounds = rp;
    }
    mid.removeFromLeft (16);
    suggestPanel = mid;
    {
        auto sp = suggestPanel.reduced (12, 10);
        sp.removeFromTop (18);
        suggestText.setBounds (sp.removeFromTop (26));
        sp.removeFromTop (6);
        auto btns = sp.removeFromTop (28);
        useKeyBtn.setBounds (btns.removeFromLeft (84));
        btns.removeFromLeft (8);
        usePresetBtn.setBounds (btns.removeFromLeft (96));
    }

    r.removeFromTop (26);
    auto combos = r.removeFromTop (44);
    auto placeCombo = [&combos] (juce::ComboBox& c, int w)
    {
        c.setBounds (combos.removeFromLeft (w).withSizeKeepingCentre (w, 28));
        combos.removeFromLeft (14);
    };
    placeCombo (modeBox, 96);
    placeCombo (keyBox, 84);
    placeCombo (scaleBox, 140);
    placeCombo (responseBox, 116);

    r.removeFromTop (30);
    auto knobRow = r.removeFromTop (150);
    const int kw = knobRow.getWidth() / 4;
    auto placeKnob = [&knobRow, kw] (juce::Slider& s)
    {
        s.setBounds (knobRow.removeFromLeft (kw).reduced (8));
    };
    placeKnob (sensitivity);
    placeKnob (smoothing);
    placeKnob (octave);
    placeKnob (drumSens);
}

void VoxMidiAudioProcessorEditor::paint (juce::Graphics& g)
{
    using L = VoxLookAndFeel;
    g.fillAll (L::bg);

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
    const bool flashing = (drumFlashTicks > 0);
    {
        auto rb = readoutBounds.reduced (10);
        auto sub = rb.removeFromBottom (22);
        g.setColour (active ? L::accent : L::dim);
        g.setFont (juce::Font (52.0f, juce::Font::bold));
        g.drawText (noteName (note), rb, juce::Justification::centred);

        juce::String subText;
        if (flashing)         subText = drumName (drumNote);
        else if (active)      subText = juce::String (freq, 1) + " Hz";
        else                  subText = "listening...";
        g.setColour (flashing ? L::accent : L::dim);
        g.setFont (flashing ? juce::Font (15.0f, juce::Font::bold) : juce::Font (13.0f));
        g.drawText (subText, sub, juce::Justification::centred);
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

    auto caption = [&] (juce::Component& c, const juce::String& t)
    {
        auto b = c.getBounds();
        g.setColour (L::dim);
        g.setFont (11.0f);
        g.drawText (t, b.getX(), b.getY() - 16, b.getWidth(), 14, juce::Justification::centred);
    };
    caption (modeBox, "MODE");
    caption (keyBox, "KEY");
    caption (scaleBox, "SCALE");
    caption (responseBox, "RESPONSE");
    caption (sensitivity, "SENSITIVITY");
    caption (smoothing, "SMOOTHING");
    caption (octave, "OCTAVE");
    caption (drumSens, "DRUM SENS");
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

    // drum-hit flash
    const int hc = processor.drumHitCount.load();
    if (hc != lastSeenHit)
    {
        lastSeenHit = hc;
        drumNote = processor.lastDrumNote.load();
        drumFlashTicks = 10;
        changed = true;
    }
    if (drumFlashTicks > 0) { --drumFlashTicks; changed = true; }

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
