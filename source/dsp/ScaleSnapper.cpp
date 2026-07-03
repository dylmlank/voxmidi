#include "ScaleSnapper.h"

#include <cstdlib>

namespace vox
{
    namespace
    {
        struct ScaleDef { const int* intervals; int count; };

        const int CHROM[]   = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        const int MAJOR[]   = { 0, 2, 4, 5, 7, 9, 11 };
        const int MINOR[]   = { 0, 2, 3, 5, 7, 8, 10 };
        const int DORIAN[]  = { 0, 2, 3, 5, 7, 9, 10 };
        const int MIXO[]    = { 0, 2, 4, 5, 7, 9, 10 };
        const int PENTMAJ[] = { 0, 2, 4, 7, 9 };
        const int PENTMIN[] = { 0, 3, 5, 7, 10 };
        const int BLUES[]   = { 0, 3, 5, 6, 7, 10 };
        const int HARMIN[]  = { 0, 2, 3, 5, 7, 8, 11 };

        const ScaleDef SCALES[] = {
            { CHROM, 12 }, { MAJOR, 7 }, { MINOR, 7 }, { DORIAN, 7 }, { MIXO, 7 },
            { PENTMAJ, 5 }, { PENTMIN, 5 }, { BLUES, 6 }, { HARMIN, 7 }
        };
        const int NUM_SCALES = 9;
    }

    int snapToScale (int midiNote, int keyRoot, int scaleIndex)
    {
        if (midiNote < 0)
            return midiNote;
        if (scaleIndex <= 0 || scaleIndex >= NUM_SCALES) // 0 = Chromatic: no change
            return midiNote;

        const ScaleDef& s = SCALES[scaleIndex];
        const int pc = (((midiNote - keyRoot) % 12) + 12) % 12;

        int bestDelta = 127;
        for (int i = 0; i < s.count; ++i)
            for (int k = -12; k <= 12; k += 12)
            {
                const int delta = (s.intervals[i] + k) - pc;
                if (std::abs (delta) < std::abs (bestDelta))
                    bestDelta = delta;
            }

        return midiNote + bestDelta;
    }

    int scaleDegreeUp (int midiNote, int keyRoot, int scaleIndex, int degrees)
    {
        if (scaleIndex <= 0 || scaleIndex >= NUM_SCALES)
            return midiNote + degrees; // Chromatic fallback (not used for chromatic chords)

        const ScaleDef& s = SCALES[scaleIndex];
        const int snapped = snapToScale (midiNote, keyRoot, scaleIndex);
        const int pc = (((snapped - keyRoot) % 12) + 12) % 12;

        int idx = 0;
        for (int i = 0; i < s.count; ++i)
            if (s.intervals[i] == pc) { idx = i; break; }

        const int octaveBase = snapped - keyRoot - pc; // multiple of 12
        const int target = idx + degrees;
        const int octShift = target / s.count;
        const int deg = target % s.count;
        return keyRoot + octaveBase + s.intervals[deg] + 12 * octShift;
    }
}
