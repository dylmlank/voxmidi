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
    chordBox.addItemList ({ "Off", "Octave", "Power", "Triad", "7th" }, 1);
    patternBox.addItemList ({ "Up", "Down", "Up-Down", "Random" }, 1);
    rateBox.addItemList ({ "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, 1);
    modSourceBox.addItemList ({ "Off", "Vibrato", "Dynamics" }, 1);
    velModeBox.addItemList ({ "Dynamic", "Fixed" }, 1);
    for (auto* c : { &modeBox, &keyBox, &scaleBox, &responseBox, &chordBox, &patternBox, &rateBox,
                     &modSourceBox, &velModeBox })
        addAndMakeVisible (c);
    modeAtt      = std::make_unique<CA> (processor.apvts, "mode", modeBox);
    keyAtt       = std::make_unique<CA> (processor.apvts, "key", keyBox);
    scaleAtt     = std::make_unique<CA> (processor.apvts, "scale", scaleBox);
    respAtt      = std::make_unique<CA> (processor.apvts, "response", responseBox);
    chordAtt     = std::make_unique<CA> (processor.apvts, "chord", chordBox);
    patternAtt   = std::make_unique<CA> (processor.apvts, "arpPattern", patternBox);
    rateAtt      = std::make_unique<CA> (processor.apvts, "arpRate", rateBox);
    modSourceAtt = std::make_unique<CA> (processor.apvts, "modSource", modSourceBox);
    velModeAtt   = std::make_unique<CA> (processor.apvts, "velMode", velModeBox);

    auto initKnob = [this] (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
        addAndMakeVisible (s);
    };
    for (auto* s : { &sensitivity, &smoothing, &drumSens, &octave, &arpOctaves, &arpGate, &transpose, &tempo,
                     &bendRange, &glide, &modCC, &modAmount, &velFixed, &velCurve })
        initKnob (*s);
    sensAtt      = std::make_unique<SA> (processor.apvts, "sensitivity", sensitivity);
    smoothAtt    = std::make_unique<SA> (processor.apvts, "smoothing", smoothing);
    drumSensAtt  = std::make_unique<SA> (processor.apvts, "drumSensitivity", drumSens);
    octaveAtt    = std::make_unique<SA> (processor.apvts, "octave", octave);
    arpOctAtt    = std::make_unique<SA> (processor.apvts, "arpOctaves", arpOctaves);
    arpGateAtt   = std::make_unique<SA> (processor.apvts, "arpGate", arpGate);
    transposeAtt = std::make_unique<SA> (processor.apvts, "transpose", transpose);
    tempoAtt     = std::make_unique<SA> (processor.apvts, "tempo", tempo);
    bendRangeAtt = std::make_unique<SA> (processor.apvts, "bendRange", bendRange);
    glideAtt     = std::make_unique<SA> (processor.apvts, "glide", glide);
    modCCAtt     = std::make_unique<SA> (processor.apvts, "modCC", modCC);
    modAmountAtt = std::make_unique<SA> (processor.apvts, "modAmount", modAmount);
    velFixedAtt  = std::make_unique<SA> (processor.apvts, "velFixed", velFixed);
    velCurveAtt  = std::make_unique<SA> (processor.apvts, "velCurve", velCurve);

    holdBtn.setClickingTogglesState (true);
    arpBtn.setClickingTogglesState (true);
    holdBtn.setColour (juce::TextButton::buttonOnColourId, VoxLookAndFeel::accent);
    arpBtn.setColour (juce::TextButton::buttonOnColourId, VoxLookAndFeel::accent);
    addAndMakeVisible (holdBtn);
    addAndMakeVisible (arpBtn);
    holdAtt = std::make_unique<BA> (processor.apvts, "hold", holdBtn);
    arpAtt  = std::make_unique<BA> (processor.apvts, "arp", arpBtn);

    bendBtn.setClickingTogglesState (true);
    bendBtn.setColour (juce::TextButton::buttonOnColourId, VoxLookAndFeel::accent);
    addAndMakeVisible (bendBtn);
    bendAtt = std::make_unique<BA> (processor.apvts, "bend", bendBtn);

    auto nudgeOctave = [this] (float delta)
    {
        const float v = processor.apvts.getRawParameterValue ("octave")->load();
        processor.setParamValue ("octave", juce::jlimit (-2.0f, 2.0f, v + delta));
    };
    octMinusBtn.onClick = [nudgeOctave] { nudgeOctave (-1.0f); };
    octPlusBtn.onClick  = [nudgeOctave] { nudgeOctave (1.0f); };
    addAndMakeVisible (octMinusBtn);
    addAndMakeVisible (octPlusBtn);

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

    setSize (660, 872);
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
    presetBox.setBounds (header.removeFromRight (150).withSizeKeepingCentre (150, 30));
    header.removeFromRight (12);
    octPlusBtn.setBounds (header.removeFromRight (46).withSizeKeepingCentre (46, 28));
    header.removeFromRight (6);
    octMinusBtn.setBounds (header.removeFromRight (46).withSizeKeepingCentre (46, 28));

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

    r.removeFromTop (14);
    inputHeader = r.removeFromTop (16);
    r.removeFromTop (6);
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

    r.removeFromTop (26);
    auto ik = r.removeFromTop (116);
    const int ikw = ik.getWidth() / 3;
    sensitivity.setBounds (ik.removeFromLeft (ikw).reduced (8));
    smoothing.setBounds (ik.removeFromLeft (ikw).reduced (8));
    drumSens.setBounds (ik.removeFromLeft (ikw).reduced (8));

    r.removeFromTop (8);
    perfHeader = r.removeFromTop (16);
    r.removeFromTop (6);
    auto pa = r.removeFromTop (44);
    auto placeWide = [&pa] (juce::Component& c, int w)
    {
        c.setBounds (pa.removeFromLeft (w).withSizeKeepingCentre (w, 28));
        pa.removeFromLeft (12);
    };
    placeWide (holdBtn, 84);
    placeWide (chordBox, 104);
    placeWide (arpBtn, 70);
    placeWide (patternBox, 116);
    placeWide (rateBox, 96);

    r.removeFromTop (26);
    auto pk = r.removeFromTop (116);
    const int pkw = pk.getWidth() / 5;
    arpOctaves.setBounds (pk.removeFromLeft (pkw).reduced (8));
    arpGate.setBounds (pk.removeFromLeft (pkw).reduced (8));
    transpose.setBounds (pk.removeFromLeft (pkw).reduced (8));
    octave.setBounds (pk.removeFromLeft (pkw).reduced (8));
    tempo.setBounds (pk.removeFromLeft (pkw).reduced (8));

    r.removeFromTop (8);
    exprHeader = r.removeFromTop (16);
    r.removeFromTop (6);
    auto ea = r.removeFromTop (44);
    auto placeExpr = [&ea] (juce::Component& c, int w)
    {
        c.setBounds (ea.removeFromLeft (w).withSizeKeepingCentre (w, 28));
        ea.removeFromLeft (12);
    };
    placeExpr (bendBtn, 84);
    placeExpr (modSourceBox, 120);
    placeExpr (velModeBox, 120);

    r.removeFromTop (26);
    auto ek = r.removeFromTop (116);
    const int ekw = ek.getWidth() / 6;
    bendRange.setBounds (ek.removeFromLeft (ekw).reduced (6));
    glide.setBounds (ek.removeFromLeft (ekw).reduced (6));
    modCC.setBounds (ek.removeFromLeft (ekw).reduced (6));
    modAmount.setBounds (ek.removeFromLeft (ekw).reduced (6));
    velFixed.setBounds (ek.removeFromLeft (ekw).reduced (6));
    velCurve.setBounds (ek.removeFromLeft (ekw).reduced (6));
}

void VoxMidiAudioProcessorEditor::paint (juce::Graphics& g)
{
    using L = VoxLookAndFeel;
    g.fillAll (L::bg);

    g.setColour (L::text);
    g.setFont (juce::Font (22.0f, juce::Font::bold));
    g.drawText ("VoxMIDI", 16, 12, 200, 26, juce::Justification::centredLeft);

    // section labels
    g.setColour (L::accent);
    g.setFont (juce::Font (12.0f, juce::Font::bold));
    g.drawText ("INPUT", inputHeader, juce::Justification::centredLeft);
    g.drawText ("PERFORMANCE", perfHeader, juce::Justification::centredLeft);
    g.drawText ("EXPRESSION", exprHeader, juce::Justification::centredLeft);

    // readout panel
    g.setColour (L::panel);
    g.fillRoundedRectangle (readoutPanel.toFloat(), 8.0f);
    const bool active = (note >= 0);
    const bool flashing = (drumFlashTicks > 0);
    {
        auto rb = readoutBounds.reduced (10);
        auto sub = rb.removeFromBottom (22);
        g.setColour (active ? L::accent : L::dim);
        g.setFont (juce::Font (50.0f, juce::Font::bold));
        g.drawText (noteName (note), rb, juce::Justification::centred);

        juce::String subText;
        if (flashing)    subText = drumName (drumNote);
        else if (active) subText = juce::String (freq, 1) + " Hz";
        else             subText = "listening...";
        g.setColour (flashing ? L::accent : L::dim);
        g.setFont (flashing ? juce::Font (15.0f, juce::Font::bold) : juce::Font (13.0f));
        g.drawText (subText, sub, juce::Justification::centred);
    }
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
        g.drawText (t, b.getX(), b.getY() - 15, b.getWidth(), 13, juce::Justification::centred);
    };
    caption (modeBox, "MODE");
    caption (keyBox, "KEY");
    caption (scaleBox, "SCALE");
    caption (responseBox, "RESPONSE");
    caption (sensitivity, "SENSITIVITY");
    caption (smoothing, "SMOOTHING");
    caption (drumSens, "DRUM SENS");
    caption (chordBox, "CHORD");
    caption (patternBox, "ARP PATTERN");
    caption (rateBox, "ARP RATE");
    caption (arpOctaves, "OCTAVES");
    caption (arpGate, "GATE");
    caption (transpose, "TRANSPOSE");
    caption (octave, "OCTAVE");
    caption (tempo, "TEMPO");
    caption (modSourceBox, "MOD SOURCE");
    caption (velModeBox, "VELOCITY");
    caption (bendRange, "BEND RANGE");
    caption (glide, "GLIDE");
    caption (modCC, "MOD CC");
    caption (modAmount, "MOD AMT");
    caption (velFixed, "FIXED VEL");
    caption (velCurve, "VEL CURVE");
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
