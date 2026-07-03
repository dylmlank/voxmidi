#include "Performance.h"
#include "dsp/ScaleSnapper.h"

void Performance::setup (double sr)
{
    sampleRate = sr;
    root = -1;
    activeCount = 0;
    curArpNote = -1;
    seqLen = 0;
    seqIdx = -1;
    stepCountdown = 0.0;
    dirty = needFlush = false;
    recalcStep = true;
}

void Performance::reset (juce::MidiBuffer& midi, juce::MidiOutput* out)
{
    flushAll (0, midi, out);
    root = -1;
    dirty = false;
}

void Performance::setArp (bool on, int pattern, int rateIdx, int octaves, float gate)
{
    if (on != arpOn) { arpOn = on; needFlush = true; dirty = true; }
    arpPattern = pattern;
    arpOctaves = juce::jlimit (1, 3, octaves);
    arpGate = juce::jlimit (0.05f, 1.0f, gate);
    if (rateIdx != arpRateIdx) { arpRateIdx = rateIdx; recalcStep = true; }
}

void Performance::noteInput (int inKeyNote, int velocity)
{
    if (inKeyNote < 0)
    {
        if (! hold && root != -1) { root = -1; dirty = true; } // release (unless latched)
    }
    else
    {
        if (inKeyNote != root) { root = inKeyNote; dirty = true; }
        vel = velocity;
    }
}

void Performance::recomputeStep()
{
    // rate index: 0=1/4, 1=1/8, 2=1/8T, 3=1/16, 4=1/16T, 5=1/32
    static const double beats[] = { 1.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 };
    const double b = beats[juce::jlimit (0, 5, arpRateIdx)];
    const double secondsPerBeat = 60.0 / juce::jmax (1.0, tempo);
    samplesPerStep = juce::jmax (1.0, sampleRate * secondsPerBeat * b);
}

int Performance::buildChord (int rootNote, int* out) const
{
    switch (chordType)
    {
        case 1: out[0] = rootNote; out[1] = rootNote + 12; return 2;                 // Octave
        case 2: out[0] = rootNote; out[1] = rootNote + 7;  return 2;                 // Power
        case 3:                                                                       // Triad
        case 4:                                                                       // 7th
        {
            int third, fifth, seventh;
            if (scale == 0) { third = rootNote + 4; fifth = rootNote + 7; seventh = rootNote + 10; }
            else
            {
                third   = vox::scaleDegreeUp (rootNote, key, scale, 2);
                fifth   = vox::scaleDegreeUp (rootNote, key, scale, 4);
                seventh = vox::scaleDegreeUp (rootNote, key, scale, 6);
            }
            out[0] = rootNote; out[1] = third; out[2] = fifth;
            if (chordType == 4) { out[3] = seventh; return 4; }
            return 3;
        }
        default: out[0] = rootNote; return 1;                                         // Off
    }
}

void Performance::rebuildArpSeq()
{
    int base[4];
    const int c = (root >= 0) ? buildChord (root, base) : 0;
    seqLen = 0;
    for (int oct = 0; oct < arpOctaves; ++oct)
        for (int i = 0; i < c; ++i)
            if (seqLen < (int) seq.size())
                seq[(size_t) seqLen++] = base[i] + 12 * oct;

    seqIdx = (arpPattern == 1) ? seqLen : -1; // Down starts from the top
    seqDir = 1;
}

int Performance::nextSeqNote()
{
    if (seqLen <= 0) return -1;
    switch (arpPattern)
    {
        case 1: seqIdx = (seqIdx - 1 + seqLen) % seqLen; break;                       // Down
        case 2:                                                                        // Up-Down
            seqIdx += seqDir;
            if (seqIdx >= seqLen - 1) { seqIdx = juce::jmax (0, seqLen - 1); seqDir = -1; }
            else if (seqIdx <= 0)     { seqIdx = 0; seqDir = 1; }
            break;
        case 3:                                                                        // Random
            rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
            seqIdx = (int) (rng % (std::uint32_t) seqLen);
            break;
        default: seqIdx = (seqIdx + 1) % seqLen; break;                                // Up
    }
    return seq[(size_t) juce::jlimit (0, seqLen - 1, seqIdx)];
}

void Performance::emitOn (int note, int v, int off, juce::MidiBuffer& midi, juce::MidiOutput* out)
{
    const auto m = juce::MidiMessage::noteOn (channel, note, (juce::uint8) juce::jlimit (1, 127, v));
    midi.addEvent (m, off);
    if (out) out->sendMessageNow (m);
}

void Performance::emitOff (int note, int off, juce::MidiBuffer& midi, juce::MidiOutput* out)
{
    const auto m = juce::MidiMessage::noteOff (channel, note);
    midi.addEvent (m, off);
    if (out) out->sendMessageNow (m);
}

void Performance::flushAll (int off, juce::MidiBuffer& midi, juce::MidiOutput* out)
{
    for (int i = 0; i < activeCount; ++i)
        emitOff (active[(size_t) i], off, midi, out);
    activeCount = 0;
    if (curArpNote >= 0) { emitOff (curArpNote, off, midi, out); curArpNote = -1; }
}

void Performance::processSample (int off, juce::MidiBuffer& midi, juce::MidiOutput* out)
{
    if (needFlush) { flushAll (off, midi, out); needFlush = false; dirty = true; }
    if (recalcStep) { recomputeStep(); recalcStep = false; }

    if (arpOn)
    {
        if (dirty) { rebuildArpSeq(); dirty = false; stepCountdown = 0.0; }

        if (root < 0)
        {
            if (curArpNote >= 0) { emitOff (curArpNote, off, midi, out); curArpNote = -1; }
            return;
        }

        if (curArpNote >= 0 && --gateCountdown <= 0)
        {
            emitOff (curArpNote, off, midi, out);
            curArpNote = -1;
        }

        stepCountdown -= 1.0;
        if (stepCountdown <= 0.0)
        {
            stepCountdown += samplesPerStep;
            if (curArpNote >= 0) { emitOff (curArpNote, off, midi, out); curArpNote = -1; }
            const int n = nextSeqNote();
            if (n >= 0)
            {
                const int sn = clampShift (n);
                emitOn (sn, vel, off, midi, out);
                curArpNote = sn;
                gateCountdown = juce::jmax (1, (int) (arpGate * samplesPerStep));
            }
        }
    }
    else if (dirty)
    {
        int tgt[4];
        int cnt = (root >= 0) ? buildChord (root, tgt) : 0;
        for (int i = 0; i < cnt; ++i) tgt[i] = clampShift (tgt[i]);

        // note-offs for active notes no longer in the target
        for (int a = 0; a < activeCount; )
        {
            bool keep = false;
            for (int i = 0; i < cnt; ++i) if (tgt[i] == active[(size_t) a]) { keep = true; break; }
            if (keep) { ++a; }
            else
            {
                emitOff (active[(size_t) a], off, midi, out);
                active[(size_t) a] = active[(size_t) (activeCount - 1)];
                --activeCount;
            }
        }
        // note-ons for target notes not yet active
        for (int i = 0; i < cnt; ++i)
        {
            bool on = false;
            for (int a = 0; a < activeCount; ++a) if (active[(size_t) a] == tgt[i]) { on = true; break; }
            if (! on && activeCount < (int) active.size())
            {
                emitOn (tgt[i], vel, off, midi, out);
                active[(size_t) activeCount++] = tgt[i];
            }
        }
        dirty = false;
    }
}
