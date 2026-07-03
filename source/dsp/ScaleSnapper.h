#pragma once

namespace vox
{
    // Snap a MIDI note to the nearest note of a key/scale.
    // scaleIndex matches the UI list: 0 = Chromatic (no snap), 1 Major, 2 Minor,
    // 3 Dorian, 4 Mixolydian, 5 Pentatonic Maj, 6 Pentatonic Min, 7 Blues, 8 Harmonic Min.
    // keyRoot is 0 = C .. 11 = B. JUCE-free.
    int snapToScale (int midiNote, int keyRoot, int scaleIndex);

    // Return the note `degrees` scale-steps above midiNote within the key/scale
    // (e.g. degrees=2 -> the diatonic third, 4 -> fifth, 6 -> seventh). Used to
    // build in-key chords. For Chromatic it falls back to a semitone offset.
    int scaleDegreeUp (int midiNote, int keyRoot, int scaleIndex, int degrees);
}
