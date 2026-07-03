#pragma once

#include <JuceHeader.h>

// Clean modern dark theme for VoxMIDI: flat panels, a single teal accent, thin arc knobs.
class VoxLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VoxLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;

    static const juce::Colour bg, panel, accent, text, dim;
};
