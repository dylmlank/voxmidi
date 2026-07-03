#include "VoxLookAndFeel.h"

#include <cmath>

const juce::Colour VoxLookAndFeel::bg     { 0xff141618 };
const juce::Colour VoxLookAndFeel::panel  { 0xff1e2124 };
const juce::Colour VoxLookAndFeel::accent { 0xff34d0c0 };
const juce::Colour VoxLookAndFeel::text   { 0xffe6e8ea };
const juce::Colour VoxLookAndFeel::dim    { 0xff6b7178 };

VoxLookAndFeel::VoxLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, bg);

    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    setColour (juce::ComboBox::backgroundColourId, panel);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff2c3036));
    setColour (juce::ComboBox::arrowColourId, dim);

    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha (0.30f));
    setColour (juce::PopupMenu::highlightedTextColourId, text);

    setColour (juce::TextButton::buttonColourId, panel);
    setColour (juce::TextButton::textColourOffId, text);

    setColour (juce::Label::textColourId, dim);
}

void VoxLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float startAngle, float endAngle,
                                       juce::Slider&)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (5.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float thick = 4.0f;
    const float r = radius - thick;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    juce::Path track;
    track.addCentredArc (cx, cy, r, r, 0.0f, startAngle, endAngle, true);
    g.setColour (panel.brighter (0.12f));
    g.strokePath (track, juce::PathStrokeType (thick, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (cx, cy, r, r, 0.0f, startAngle, angle, true);
    g.setColour (accent);
    g.strokePath (value, juce::PathStrokeType (thick, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    const float tx = cx + r * std::sin (angle);
    const float ty = cy - r * std::cos (angle);
    g.setColour (text);
    g.fillEllipse (tx - 3.0f, ty - 3.0f, 6.0f, 6.0f);
}
