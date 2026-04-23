# TODO.md

B33p roadmap. Each phase is a shippable milestone. Pick tasks **only from the current phase**. Check items off when done; do not delete them.

> **Current phase: Phase 1 — Voice DSP core**

---

## Phase 0 — Project scaffolding

Goal: an empty standalone app window opens on all three OSes, built via CMake, tests run in CI.

- [x] `CMakeLists.txt` at repo root, C++17, project name `B33p`
- [x] JUCE fetched via CPM, pinned to a specific release tag (8.0.12)
- [x] Standalone target builds and launches an empty `juce::DocumentWindow` titled "B33p"
- [x] Directory skeleton: `Source/{Core,DSP,Pattern,State,UI}`, `Tests/`, `docs/images/`
- [x] `.gitignore` covers build dirs, IDE files, OS files
- [x] `LICENSE` file (GPL-3.0) at repo root
- [x] GitHub Actions workflow: build matrix for macOS, Windows, Linux (tests wired in Phase 1)
- [x] README build instructions verified on host OS (macOS)

## Phase 1 — Voice DSP core

Goal: a `Voice` class can be instantiated from a test harness and produce audio. No UI yet.

- [x] Catch2 fetched via CPM for tests, wired into CTest and the CI workflow
- [x] `Oscillator` class: sine, square, triangle, saw, noise. Unit tested for expected waveform shape.
- [ ] `AmpEnvelope` ADSR class, unit tested (attack ramp, sustain, release tail, retrigger)
- [ ] `PitchEnvelope` interface accepts an arbitrary curve (point list with interpolation)
- [ ] `LowpassFilter` wrapping `juce::dsp::IIR` (cutoff + resonance), unit tested
- [ ] `Bitcrush` effect (sample-rate and bit-depth reduction), unit tested
- [ ] `Distortion` effect (waveshaper), unit tested
- [ ] `Voice` class composing: oscillator → pitch env → amp env → filter → bitcrush → distortion → gain (fixed order)
- [ ] Full parameter layout defined in `AudioProcessorValueTreeState`
- [ ] Test harness: CLI or unit test that renders a voice to WAV for listening checks

## Phase 2 — Voice UI

Goal: a user can audition and design a single voice with every parameter exposed.

- [ ] Layout: oscillator section, amp env section, filter section, effects section, master
- [ ] Waveform selector (dropdown)
- [ ] Rotary sliders bound to APVTS for all continuous parameters
- [ ] ADSR visualizer for amp envelope
- [ ] **Drawable pitch envelope** component — click to add points, drag to move, right-click to delete, curve interpolation
  - **Contingency:** if the drawable version is slipping, ship Phase 2 with an ADSR-style pitch-env UI against the same `PitchEnvelope` interface, then deliver drawable as Phase 2.5. Do not block the phase on it.
- [ ] "Audition" button plays the voice at a fixed pitch
- [ ] Spacebar = audition

## Phase 3 — Randomization system

Goal: every voice parameter has a dice button and a lock; a top-level dice rerolls everything unlocked; undo works.

- [ ] `Randomizable<T>` wrapper (or equivalent): owns a range, a lock flag, a `roll()` that respects the lock
- [ ] Hardcoded parameter-range table covering every voice parameter
- [ ] Dice UI element + lock toggle beside every parameter
- [ ] Top-level "Dice All" button rerolls all non-locked parameters in one `UndoManager` transaction
- [ ] Unit tests for lock behaviour and range clamping

## Phase 4 — Pattern sequencer

Goal: place and edit events on up to 4 lanes, loop-play the pattern.

- [ ] `Pattern` data model: lanes, events (start, duration, pitch offset)
- [ ] 10-second maximum pattern length enforced
- [ ] Timeline UI: ruler, 4 lanes, playhead
- [ ] Click-drag to create an event; drag edges to resize; drag body to move
- [ ] Delete/Backspace removes selected event
- [ ] Grid-snap dropdown: 1/4, 1/8, 1/16, 1/32, off
- [ ] Playback engine: triggers the voice on event start, applies pitch offset
- [ ] Loop toggle
- [ ] Unit tests for pattern model (insertion order, overlap handling, snap math)

## Phase 5 — WAV export

Goal: offline-render the pattern to a WAV file the user picks.

- [ ] Offline render path (no realtime audio device) — deterministic given the same project
- [ ] Export dialog: file picker, sample rate, bit depth, channels
  - **Sample rates:** 8 / 11.025 / 16 / 22.05 / 44.1 / 48 kHz
  - **Bit depths:** 8-bit unsigned PCM, 16-bit, 24-bit
  - **Channels:** mono / stereo-duplicated
  - 8-bit + low sample rates are the intentional lo-fi / retro path. Effective resolutions below 8-bit are produced via the Phase 1 bitcrusher rendering into the chosen container.
- [ ] Render matches pattern length exactly; trailing tail captured until voice silence threshold
- [ ] Progress indicator for renders > 1 second

## Phase 6 — Project save / load

Goal: round-trip a full project (voice + pattern) through a `.beep` file.

- [ ] `ValueTree` schema for the full project, versioned from v1
- [ ] "Save As" dialog writes a `.beep`
- [ ] "Open" dialog loads a `.beep` and restores state exactly
- [ ] Unsaved-changes indicator in the window title
- [ ] macOS `.beep` file-type association in the app bundle
- [ ] Migration-function stub for future v1 → v2 transitions

## Phase 7 — MVP polish & release

Goal: v0.1 is shippable.

- [ ] App icon (placeholder OK if no final asset)
- [ ] About box: version, author, license, JUCE credit
- [ ] README screenshots captured into `docs/images/`
- [ ] macOS `.dmg`, Windows installer, Linux AppImage or tarball
- [ ] Tag `v0.1.0`

---

## Chores / tech debt

Cross-cutting work that isn't tied to any single phase. Review at the start of every session — if any item has a time-sensitive deadline approaching, flag it to the user before starting feature work.

- [ ] Bump `actions/checkout` and `actions/cache` to `@v5` before June 2, 2026 (Node 20 deprecation — GitHub Actions warning surfaced on every run)

---

## Post-MVP roadmap

Ordered roughly by priority, not commitment. Each item becomes its own phase when prioritized.

### Sound-design power

- [ ] **Custom drawn waveforms** — user draws a single-cycle oscillator shape (high-priority post-MVP)
- [ ] **Wavetable oscillator** — morph between waveforms
- [ ] **FM oscillator** — two-operator FM with ratio + depth
- [ ] **Ring modulation** — second oscillator + ring-mod mix
- [ ] **Additional filters** — formant, comb, bandpass, highpass
- [ ] **Additional effects** — chorus, reverb, delay, flanger, phaser
- [ ] **LFOs and a modulation matrix** — any source → any destination
- [ ] **Per-event parameter overrides** beyond pitch (e.g., filter cutoff per hit)
- [ ] **Event probability, ratcheting, humanize**

### Workflow

- [ ] **Multiple voices per project** — each lane references its own voice
- [ ] **Preset browser** with tags, favorites, search
- [ ] **Generator presets** — one-click starting points (droid chatter, alarms, weapon charge, UI beeps)
- [ ] **User-configurable randomization ranges and distributions**
- [ ] **MIDI input** for auditioning
- [ ] **Polyphony** for voices that benefit from it
- [ ] **Batch export** — render N dice-rolled variations in one shot

### Distribution

- [ ] **VST3 plugin build**
- [ ] **AU plugin build** (macOS)
- [ ] **AAX plugin build** (Pro Tools)
- [ ] **Additional export formats** — FLAC, OGG, AIFF, 96 kHz, IMA ADPCM WAV (true 4-bit)
