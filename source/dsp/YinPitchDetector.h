#pragma once

#include <vector>

// Monophonic pitch detection via the YIN algorithm (de Cheveigné & Kawahara, 2002).
// Deliberately JUCE-free so it can be unit-tested on its own and swapped later
// (pYIN / CREPE) without touching the plugin. Allocates once in setup(); process()
// is allocation-free and safe to call on the audio thread.
class YinPitchDetector
{
public:
    void setup (double sampleRate, int windowSize);
    void setThreshold (float t) { threshold = t; }

    // Analyse one window; numSamples must equal the windowSize passed to setup().
    // Returns the fundamental frequency in Hz, or 0.0f if no periodic pitch is found.
    // outProbability (0..1) is a confidence estimate (1 - aperiodicity at the pick).
    float process (const float* buffer, int numSamples, float& outProbability);

private:
    float parabolicInterpolation (int tauEstimate) const;

    double sampleRate = 44100.0;
    int halfWindow = 1024;
    float threshold = 0.15f;
    std::vector<float> yin; // cumulative mean normalized difference, size halfWindow
};
