#pragma once

// Turns a stream of raw pitch estimates into a stable current note. Gates on input
// level and detector confidence, then requires a candidate note to persist for a few
// frames (hysteresis) before committing — this kills the flicker and octave jumps that
// make raw pitch detection unusable. JUCE-free. M3 will read note changes to emit MIDI.
class NoteTracker
{
public:
    void setup (double sampleRate);

    // Feed one analysis frame. Returns the current stable MIDI note (0..127), or -1
    // for "no note". frequencyHz <= 0 means the detector found nothing this frame.
    int process (float frequencyHz, float probability, float rms);

    void setLevelGate (float linear) { levelGate = linear; }
    void setConfidenceGate (float p) { confidenceGate = p; }
    void setHysteresisFrames (int n) { hysteresisFrames = n; }

    int   getCurrentNote() const { return currentNote; }
    float getCurrentFrequency() const { return currentFrequency; }

private:
    static int frequencyToMidi (float hz);

    float levelGate = 0.02f;      // ~ -34 dBFS: below this = silence
    float confidenceGate = 0.90f; // YIN probability threshold
    int   hysteresisFrames = 3;   // frames a candidate must hold before it commits

    int   currentNote = -1;
    float currentFrequency = 0.0f;

    int candidateNote = -1;
    int candidateCount = 0;
};
