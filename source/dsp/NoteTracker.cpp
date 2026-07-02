#include "NoteTracker.h"

#include <cmath>

void NoteTracker::setup (double)
{
    currentNote = -1;
    currentFrequency = 0.0f;
    candidateNote = -1;
    candidateCount = 0;
}

int NoteTracker::frequencyToMidi (float hz)
{
    if (hz <= 0.0f)
        return -1;
    const float m = 69.0f + 12.0f * std::log2 (hz / 440.0f);
    const int midi = (int) std::lround (m);
    return (midi < 0 || midi > 127) ? -1 : midi;
}

int NoteTracker::process (float frequencyHz, float probability, float rms)
{
    // A frame only counts as a note if it clears the level and confidence gates.
    int target = -1;
    if (rms >= levelGate && probability >= confidenceGate && frequencyHz > 0.0f)
        target = frequencyToMidi (frequencyHz);

    if (target == candidateNote)
    {
        if (candidateCount < 1'000'000)
            ++candidateCount;
    }
    else
    {
        candidateNote = target;
        candidateCount = 1;
    }

    // Commit the candidate once it has been stable long enough (applies to note-off too).
    if (candidateCount >= hysteresisFrames && candidateNote != currentNote)
        currentNote = candidateNote;

    if (currentNote >= 0 && frequencyHz > 0.0f)
        currentFrequency = frequencyHz;
    else if (currentNote < 0)
        currentFrequency = 0.0f;

    return currentNote;
}
