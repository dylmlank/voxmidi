#pragma once

namespace vox
{
    // A preset tunes the "feel" parameters (sensitivity, smoothing, response, octave).
    // Key + scale are left to the user / the recommendation engine — a preset shouldn't
    // force a musical key. response: 0 = Fast, 1 = Balanced, 2 = Accurate.
    struct Preset
    {
        const char* name;
        float sensitivity;
        float smoothing;
        int   response;
        int   octave;
    };

    static const Preset PRESETS[] = {
        { "Lead / Melody",  60.0f, 35.0f, 1,  0 },
        { "Bass",           55.0f, 55.0f, 2, -1 },
        { "Whistle",        70.0f, 25.0f, 0,  0 },
        { "Humming (soft)", 75.0f, 45.0f, 1,  0 },
        { "Vocal Chop",     60.0f, 20.0f, 0,  0 },
        { "Rap / Spoken",   50.0f, 60.0f, 1,  0 },
    };

    static const int NUM_PRESETS = 6;
}
