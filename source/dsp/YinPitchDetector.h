#pragma once

#include <vector>

// Monophonic pitch detection via the YIN algorithm (de Cheveigné & Kawahara, 2002).
// JUCE-free so it can be unit-tested and swapped later (pYIN / CREPE). Allocates for a
// max window once in setup(); process() then accepts any window size up to that max, so
// the "Response" control can change the analysis window without allocating on the audio
// thread. process() is allocation-free and safe to call in real time.
class YinPitchDetector
{
public:
    void setup (double sampleRate, int maxWindow);
    void setThreshold (float t) { threshold = t; }

    // Analyse one window (numSamples <= the maxWindow passed to setup()).
    // Returns the fundamental frequency in Hz, or 0.0f if no periodic pitch is found.
    // outProbability (0..1) is a confidence estimate (1 - aperiodicity at the pick).
    float process (const float* buffer, int numSamples, float& outProbability);

private:
    float parabolicInterpolation (int tauEstimate, int halfWindow) const;

    double sampleRate = 44100.0;
    int maxHalfWindow = 2048;
    float threshold = 0.15f;
    std::vector<float> yin; // cumulative mean normalized difference (sized to maxHalfWindow)
};
