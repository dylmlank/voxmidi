# VoxMIDI

Real-time **voice → MIDI** for FL Studio. Sing, hum, or whistle; get MIDI notes.
A focused, low-latency alternative to Vochlea Dubler, built as **one JUCE codebase**
that produces both:

- a **VST3** plugin (loads directly in FL Studio), and
- a **standalone app** that sends MIDI over a virtual port (loopMIDI) — the reliable,
  DAW-agnostic fallback, and how Dubler itself works.

## v1 scope (this milestone)
Monophonic pitch tracking → **scale-snapped** MIDI notes.
- Mic/audio in → pitch detection (YIN) → note segmentation → snap to key/scale → MIDI out.
- Controls: input gain/gate, sensitivity (confidence threshold), latency (window size),
  key + scale selector, live note/pitch readout + input meter.
- Explicitly **out of scope for v1** (later stages): pitch-bend glide, voice→CC mapping,
  beatbox drum triggers, polyphony, calibration flow.

## Architecture
```
source/
  PluginProcessor.*   JUCE AudioProcessor: audio IN bus, MIDI OUT; hosts the DSP chain
  PluginEditor.*      GUI: meter, detected note, key/scale, sensitivity, latency
  dsp/
    YinPitchDetector.*  monophonic fundamental-frequency estimate (framework-agnostic)
    NoteTracker.*       RMS gate + confidence gate + hysteresis + note on/off segmentation
    ScaleSnapper.*      snap a MIDI note to the nearest degree of the selected key/scale
```
The `dsp/` layer has **no JUCE dependency** so it can be unit-tested standalone and the
detector swapped later (YIN → pYIN → CREPE/neural) without touching the plugin shell.

## Design targets
- **Latency:** ~10–30 ms end to end. The fight: low notes need longer analysis windows
  (more lag). Window size is user-tunable so you can trade latency vs. low-note accuracy.
- **Stability:** hysteresis + confidence gating to kill the octave-jumps and jitter that
  make naive detectors unusable.
- **Accuracy win:** scale-snapping — the single biggest "feels right" improvement.

## Build (once the toolchain is installed)
Requires: Visual Studio 2022 Build Tools (C++ workload) + CMake 3.22+. JUCE is pulled
automatically by CMake (FetchContent) — nothing to install manually.
```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
# VST3 lands in build/VoxMIDI_artefacts/Release/VST3/ ; copy to FL's plugin folder or point FL's plugin search there.
# Standalone lands in build/VoxMIDI_artefacts/Release/Standalone/.
```

## Milestones
- **M0 — Toolchain:** install VS 2022 Build Tools (C++) + CMake.
- **M1 — Buildable shell:** CMake + JUCE, audio-passthrough plugin that compiles, loads in
  FL as a VST3, and opens as a standalone. Prove the pipeline before any DSP.
- **M2 — Pitch core:** YIN detector + NoteTracker; print detected notes.
- **M3 — MIDI out:** notes → MIDI, driving an FL instrument (VST3) and loopMIDI (standalone).
- **M4 — Snap + GUI:** key/scale snap, meter, sensitivity + latency controls.
- **M5 — Tuning:** latency/accuracy pass, octave-error fixes, real-voice testing.
