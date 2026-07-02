#include "YinPitchDetector.h"

#include <cmath>

void YinPitchDetector::setup (double sr, int windowSize)
{
    sampleRate = sr;
    halfWindow = windowSize / 2;
    yin.assign ((size_t) halfWindow, 0.0f);
}

float YinPitchDetector::process (const float* buffer, int numSamples, float& outProbability)
{
    outProbability = 0.0f;
    if (halfWindow < 2 || numSamples < halfWindow * 2)
        return 0.0f;

    // Step 1: difference function
    for (int tau = 0; tau < halfWindow; ++tau)
    {
        float sum = 0.0f;
        for (int j = 0; j < halfWindow; ++j)
        {
            const float delta = buffer[j] - buffer[j + tau];
            sum += delta * delta;
        }
        yin[(size_t) tau] = sum;
    }

    // Step 2: cumulative mean normalized difference
    yin[0] = 1.0f;
    float runningSum = 0.0f;
    for (int tau = 1; tau < halfWindow; ++tau)
    {
        runningSum += yin[(size_t) tau];
        yin[(size_t) tau] = (runningSum > 0.0f) ? yin[(size_t) tau] * (float) tau / runningSum
                                                : 1.0f;
    }

    // Step 3: absolute threshold — first dip below threshold, followed down to its local min
    int tauEstimate = -1;
    for (int tau = 2; tau < halfWindow; ++tau)
    {
        if (yin[(size_t) tau] < threshold)
        {
            while (tau + 1 < halfWindow && yin[(size_t) (tau + 1)] < yin[(size_t) tau])
                ++tau;
            tauEstimate = tau;
            break;
        }
    }

    if (tauEstimate == -1)
        return 0.0f; // nothing periodic enough

    outProbability = 1.0f - yin[(size_t) tauEstimate];

    // Step 4: parabolic interpolation for sub-sample precision
    const float betterTau = parabolicInterpolation (tauEstimate);
    if (betterTau <= 0.0f)
        return 0.0f;

    return (float) (sampleRate / (double) betterTau);
}

float YinPitchDetector::parabolicInterpolation (int tauEstimate) const
{
    const int x0 = (tauEstimate > 0) ? tauEstimate - 1 : tauEstimate;
    const int x2 = (tauEstimate + 1 < halfWindow) ? tauEstimate + 1 : tauEstimate;

    if (x0 == tauEstimate)
        return (yin[(size_t) tauEstimate] <= yin[(size_t) x2]) ? (float) tauEstimate : (float) x2;
    if (x2 == tauEstimate)
        return (yin[(size_t) tauEstimate] <= yin[(size_t) x0]) ? (float) tauEstimate : (float) x0;

    const float s0 = yin[(size_t) x0];
    const float s1 = yin[(size_t) tauEstimate];
    const float s2 = yin[(size_t) x2];
    const float denom = 2.0f * (2.0f * s1 - s2 - s0);
    if (std::abs (denom) < 1.0e-9f)
        return (float) tauEstimate;
    return (float) tauEstimate + (s2 - s0) / denom;
}
