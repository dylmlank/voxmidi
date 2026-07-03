#include "KeyEstimator.h"

#include <cmath>

namespace vox
{
    void KeyEstimator::reset()
    {
        for (int i = 0; i < 12; ++i)
            histogram[i] = 0.0f;
        meanNote = 0.0;
        count = 0;
    }

    void KeyEstimator::addSample (int midiNote)
    {
        if (midiNote < 0)
            return;

        for (int i = 0; i < 12; ++i)   // slow decay so it adapts as you keep singing
            histogram[i] *= 0.999f;

        const int pc = ((midiNote % 12) + 12) % 12;
        histogram[pc] += 1.0f;

        meanNote = (count == 0) ? (double) midiNote
                                : meanNote * 0.99 + (double) midiNote * 0.01;
        if (count < 1'000'000)
            ++count;
    }

    KeyEstimator::Result KeyEstimator::estimate() const
    {
        Result r;
        if (count < 24)   // need a little material first
            return r;

        // Krumhansl-Schmuckler tonal profiles
        static const double MAJ[12] = { 6.35, 2.23, 3.48, 2.33, 4.38, 4.09,
                                        2.52, 5.19, 2.39, 3.66, 2.29, 2.88 };
        static const double MIN[12] = { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53,
                                        2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };

        double hMean = 0.0;
        for (int i = 0; i < 12; ++i)
            hMean += histogram[i];
        hMean /= 12.0;

        double bestCorr = -2.0;
        int    bestKey = 0;
        bool   bestMinor = false;

        for (int key = 0; key < 12; ++key)
        {
            for (int m = 0; m < 2; ++m)
            {
                const double* prof = (m == 1) ? MIN : MAJ;
                double pMean = 0.0;
                for (int i = 0; i < 12; ++i)
                    pMean += prof[i];
                pMean /= 12.0;

                double num = 0.0, dh = 0.0, dp = 0.0;
                for (int i = 0; i < 12; ++i)
                {
                    const double h = histogram[(i + key) % 12] - hMean;
                    const double p = prof[i] - pMean;
                    num += h * p;
                    dh += h * h;
                    dp += p * p;
                }

                const double corr = (dh > 0.0 && dp > 0.0) ? num / std::sqrt (dh * dp) : -2.0;
                if (corr > bestCorr)
                {
                    bestCorr = corr;
                    bestKey = key;
                    bestMinor = (m == 1);
                }
            }
        }

        r.valid = true;
        r.key = bestKey;
        r.scaleIndex = bestMinor ? 2 : 1;
        r.strength = (float) ((bestCorr + 1.0) * 0.5); // -1..1 -> 0..1

        // Preset suggestion from vocal range center: low -> Bass, high -> Whistle, else Lead.
        const int mn = (int) meanNote;
        r.presetIndex = (mn < 50) ? 1 : (mn > 72 ? 2 : 0);
        return r;
    }
}
