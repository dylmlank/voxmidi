#include "TransientDetector.h"

#include <cmath>

namespace vox
{
    void TransientDetector::setup (double sr)
    {
        sampleRate = sr;
        aFast = 1.0f - std::exp (-1.0f / (0.002f * (float) sr));  // ~2 ms attack follower
        aSlow = 1.0f - std::exp (-1.0f / (0.150f * (float) sr));  // ~150 ms baseline
        rel   = std::exp (-1.0f / (0.050f * (float) sr));         // peak env release
        refractorySamples = (int) (0.060 * sr);                   // 60 ms debounce
        refractory = 0;
        prevX = fast = slow = bbEnv = hfEnv = 0.0f;
    }

    void TransientDetector::setSensitivity (float s01)
    {
        s01 = s01 < 0.0f ? 0.0f : (s01 > 1.0f ? 1.0f : s01);
        ratio      = 5.0f - s01 * 3.2f;      // 5.0 (strict) .. 1.8 (touchy)
        floorLevel = 0.020f - s01 * 0.018f;  // 0.020 .. 0.002 noise floor
    }

    int TransientDetector::process (float x, float& outVelocity)
    {
        const float lvl = std::fabs (x);
        const float d = x - prevX;
        prevX = x;
        const float ad = std::fabs (d);

        fast += (lvl - fast) * aFast;
        slow += (lvl - slow) * aSlow;
        bbEnv = (lvl > bbEnv * rel) ? lvl : bbEnv * rel;
        hfEnv = (ad  > hfEnv * rel) ? ad  : hfEnv * rel;

        if (refractory > 0)
        {
            --refractory;
            return -1;
        }

        if (fast > floorLevel && fast > slow * ratio)
        {
            const float brightness = hfEnv / (bbEnv + 1.0e-6f);

            // In "Both" mode, a bright-poor onset with a steady pitch is a sung note, not a drum.
            if (gateTonal && tonalConf > 0.9f && brightness < 0.5f)
                return -1;

            const int note = (brightness < 0.15f) ? 36        // kick
                                                   : (brightness < 0.45f ? 38   // snare
                                                                         : 42); // hi-hat

            float v = (fast - floorLevel) / (0.30f - floorLevel);
            v = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
            outVelocity = 60.0f + v * 67.0f; // 60..127

            refractory = refractorySamples;
            return note;
        }

        return -1;
    }
}
