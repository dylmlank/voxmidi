#pragma once

namespace vox
{
    // Listens to the stream of sung notes and suggests a musical key/scale (via the
    // Krumhansl-Schmuckler key-finding profiles) plus a preset that suits the voice's
    // range. Fed one sample at a time from the editor's timer (message thread only —
    // no locking needed). JUCE-free.
    class KeyEstimator
    {
    public:
        struct Result
        {
            bool  valid = false;
            int   key = 0;         // 0 = C .. 11 = B
            int   scaleIndex = 1;  // 1 = Major, 2 = Minor (matches the UI list)
            int   presetIndex = 0; // index into vox::PRESETS
            float strength = 0.0f; // correlation 0..1, how confident
        };

        void reset();
        void addSample (int midiNote);   // -1 (no note) is ignored
        Result estimate() const;

    private:
        float  histogram[12] = { 0 };
        double meanNote = 0.0;
        int    count = 0;
    };
}
