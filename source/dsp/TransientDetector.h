#pragma once

namespace vox
{
    // Detects short percussive onsets (clicks, tongue "tk", beatbox hits) and classifies
    // each by brightness into a GM drum note: dark -> kick (36), mid -> snare (38),
    // bright -> hi-hat (42). Per-sample, allocation-free, JUCE-free. In "Both" mode a
    // tonal gate suppresses onsets that carry a steady pitch (i.e. sung notes).
    class TransientDetector
    {
    public:
        void setup (double sampleRate);
        void setSensitivity (float s01);                 // 0..1
        void setGateTonal (bool on) { gateTonal = on; }
        void setTonalConfidence (float p) { tonalConf = p; }

        // Per sample. Returns a GM drum note on an onset, else -1.
        // outVelocity (1..127) is valid only when a note is returned.
        int process (float x, float& outVelocity);

    private:
        double sampleRate = 44100.0;
        float aFast = 0.02f, aSlow = 0.0005f, rel = 0.999f;
        float ratio = 3.0f, floorLevel = 0.006f;
        int   refractory = 0, refractorySamples = 2646; // ~60 ms

        float prevX = 0.0f, fast = 0.0f, slow = 0.0f, bbEnv = 0.0f, hfEnv = 0.0f;

        bool  gateTonal = false;
        float tonalConf = 0.0f;
    };
}
