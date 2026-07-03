#pragma once

#include <JuceHeader.h>
#include <array>
#include <cstdint>

// The "performance" layer: turns the single sung note into MIDI keyboard-style output.
// Order of effects: the incoming note is already snapped to key/scale (in-key, unshifted);
// here we optionally HOLD it, expand it into a CHORD, run an ARPEGGIATOR over the chord,
// and finally apply a global octave/transpose SHIFT on the way out. It owns note on/off
// lifecycle so nothing gets stuck. Emits to the host MIDI buffer and, if given, a loopMIDI
// output. Runs on the audio thread; allocation-free.
class Performance
{
public:
    void setup (double sampleRate);
    void reset (juce::MidiBuffer& midi, juce::MidiOutput* out); // all notes off + clear

    void setChannel (int c)          { channel = juce::jlimit (1, 16, c); }
    void setHold (bool h)            { hold = h; }
    void setChord (int c)            { if (c != chordType) { chordType = c; dirty = true; } }
    void setShift (int semis)        { if (semis != shift) { shift = semis; needFlush = true; } }
    void setKeyScale (int k, int s)  { if (k != key || s != scale) { key = k; scale = s; dirty = true; } }
    void setTempo (double bpm)       { if (std::abs (bpm - tempo) > 0.01) { tempo = bpm; recalcStep = true; } }
    void setArp (bool on, int pattern, int rateIdx, int octaves, float gate);

    void noteInput (int inKeyNote, int velocity); // -1 => released
    void processSample (int sampleOffset, juce::MidiBuffer& midi, juce::MidiOutput* out);

    int currentRootForDisplay() const { return (root >= 0) ? juce::jlimit (0, 127, root + shift) : -1; }

private:
    void emitOn  (int note, int vel, int off, juce::MidiBuffer&, juce::MidiOutput*);
    void emitOff (int note, int off, juce::MidiBuffer&, juce::MidiOutput*);
    void flushAll (int off, juce::MidiBuffer&, juce::MidiOutput*);
    int  buildChord (int rootNote, int* outNotes) const; // in-key unshifted; returns count
    void rebuildArpSeq();
    void recomputeStep();
    int  clampShift (int n) const { return juce::jlimit (0, 127, n + shift); }
    int  nextSeqNote();

    double sampleRate = 44100.0, tempo = 120.0;
    int channel = 1, chordType = 0, shift = 0, key = 0, scale = 1;
    bool hold = false, arpOn = false;
    int arpPattern = 0, arpRateIdx = 1, arpOctaves = 1;
    float arpGate = 0.5f;

    int root = -1, vel = 100; // held in-key root (unshifted); -1 = silent
    bool dirty = false, needFlush = false, recalcStep = true;

    std::array<int, 8> active { {} }; // chord-mode notes currently on (as sent, shifted)
    int activeCount = 0;

    std::array<int, 16> seq { {} };   // arp sequence (in-key, unshifted)
    int seqLen = 0, seqIdx = -1, seqDir = 1;
    double samplesPerStep = 22050.0, stepCountdown = 0.0;
    int curArpNote = -1, gateCountdown = 0;
    std::uint32_t rng = 2463534242u;
};
