# TODO.md

b33p roadmap. Each phase is a shippable milestone. Pick tasks **only from the current phase**. Check items off when done; do not delete them.

> **Current focus: post-MVP roadmap — Filter response visualizer is the next item being worked on**

---

## Phase 0 — Project scaffolding

Goal: an empty standalone app window opens on all three OSes, built via CMake, tests run in CI.

- [x] `CMakeLists.txt` at repo root, C++17, project name `B33p`
- [x] JUCE fetched via CPM, pinned to a specific release tag (8.0.12)
- [x] Standalone target builds and launches an empty `juce::DocumentWindow` titled "b33p"
- [x] Directory skeleton: `Source/{Core,DSP,Pattern,State,UI}`, `Tests/`, `docs/images/`
- [x] `.gitignore` covers build dirs, IDE files, OS files
- [x] `LICENSE` file (GPL-3.0) at repo root
- [x] GitHub Actions workflow: build matrix for macOS, Windows, Linux (tests wired in Phase 1)
- [x] README build instructions verified on host OS (macOS)

## Phase 1 — Voice DSP core

Goal: a `Voice` class can be instantiated from a test harness and produce audio. No UI yet.

- [x] Catch2 fetched via CPM for tests, wired into CTest and the CI workflow
- [x] `Oscillator` class: sine, square, triangle, saw, noise. Unit tested for expected waveform shape.
- [x] `AmpEnvelope` ADSR class, unit tested (attack ramp, sustain, release tail, retrigger)
- [x] `PitchEnvelope` interface accepts an arbitrary curve (point list with interpolation)
- [x] `LowpassFilter` wrapping `juce::dsp::IIR` (cutoff + resonance), unit tested
- [x] `Bitcrush` effect (sample-rate and bit-depth reduction), unit tested
- [x] `Distortion` effect (waveshaper), unit tested
- [x] `Voice` class composing: oscillator → pitch env → amp env → filter → bitcrush → distortion → gain (fixed order)
- [x] Full parameter layout defined in `AudioProcessorValueTreeState`
- [x] Test harness: CLI or unit test that renders a voice to WAV for listening checks

## Phase 2 — Voice UI

Goal: a user can audition and design a single voice with every parameter exposed.

- [x] Layout: oscillator section, amp env section, filter section, effects section, master
- [x] Waveform selector (dropdown)
- [x] Rotary sliders bound to APVTS for all continuous parameters
- [x] ADSR visualizer for amp envelope
- [x] **Drawable pitch envelope** component — click to add points, drag to move, right-click to delete, curve interpolation
  - **Contingency:** if the drawable version is slipping, ship Phase 2 with an ADSR-style pitch-env UI against the same `PitchEnvelope` interface, then deliver drawable as Phase 2.5. Do not block the phase on it.
- [x] "Audition" button plays the voice at a fixed pitch
- [x] Spacebar = audition

## Phase 3 — Randomization system

Goal: every voice parameter has a dice button and a lock; a top-level dice rerolls everything unlocked; undo works.

- [x] `Randomizable<T>` wrapper (or equivalent): owns a range, a lock flag, a `roll()` that respects the lock
- [x] Hardcoded parameter-range table covering every voice parameter
- [x] Dice UI element + lock toggle beside every parameter
- [x] Top-level "Dice All" button rerolls all non-locked parameters in one `UndoManager` transaction
- [x] Unit tests for lock behaviour and range clamping

## Phase 4 — Pattern sequencer

Goal: place and edit events on up to 4 lanes, loop-play the pattern.

- [x] `Pattern` data model: lanes, events (start, duration, pitch offset)
- [x] 10-second maximum pattern length enforced
- [x] Timeline UI: ruler, 4 lanes, playhead
- [x] Click-drag to create an event; drag edges to resize; drag body to move
- [x] Delete/Backspace removes selected event
- [x] Grid-size / snap dropdown in milliseconds: `10 ms`, `25 ms`, `50 ms`, `100 ms`, `250 ms`, `500 ms`, `1 s`, off (musical snap is post-MVP)
- [x] Pattern-length control: user-adjustable within `[0.1 s, 10 s]`
- [x] Playback engine: triggers the voice on event start, applies pitch offset
- [x] Loop toggle
- [x] Unit tests for pattern model (insertion order, overlap handling, snap math)

## Phase 5 — WAV export

Goal: offline-render the pattern to a WAV file the user picks.

- [x] Offline render path (no realtime audio device) — deterministic given the same project
- [x] Export dialog: file picker, sample rate, bit depth, channels
  - **Sample rates:** 8 / 11.025 / 16 / 22.05 / 44.1 / 48 kHz
  - **Bit depths:** 8-bit unsigned PCM, 16-bit, 24-bit
  - **Channels:** mono / stereo-duplicated
  - 8-bit + low sample rates are the intentional lo-fi / retro path. Effective resolutions below 8-bit are produced via the Phase 1 bitcrusher rendering into the chosen container.
- [x] Render matches pattern length exactly; trailing tail captured until voice silence threshold
- [x] Progress indicator for renders > 1 second

## Phase 6 — Project save / load

Goal: round-trip a full project (voice + pattern) through a `.beep` file.

- [x] `ValueTree` schema for the full project, versioned from v1
- [x] "Save As" dialog writes a `.beep`
- [x] "Open" dialog loads a `.beep` and restores state exactly
- [x] Unsaved-changes indicator in the window title
- [x] macOS `.beep` file-type association in the app bundle
- [x] Migration-function stub for future v1 → v2 transitions

## Phase 7 — MVP polish & release

Goal: v0.1 is shippable.

- [x] App icon (placeholder OK if no final asset)
- [x] About box: version, author, license, JUCE credit
- [x] README screenshots captured into `docs/images/`
- [x] CI uploads the built b33p binary on every run as a workflow artifact (so non-developer testers can grab a build without setting up the toolchain)
- [x] Tag `v0.1.0`

## Phase 8 — Pattern editor: per-event control + intuitive clip editing

Goal: every clip in the pattern has its own pitch offset and velocity, the editing gestures match standard DAW conventions, and a fresh user can figure out how to place / move / resize / nudge / delete clips without a manual.

Framing: today the pattern editor is "place a beep here, drag it, drag its edge". After this phase it's a real sequencing surface — the clip editing feels native (cursor changes, snap guides, both edges resize, drag between lanes), every event carries its own pitch and dynamics, and an inspector strip exposes the numbers when you need them.

### Per-event data + playback
- [x] `Event` gains a `velocity` field (0..1, default 1.0); operator== updated
- [x] Voice trigger respects per-event `pitchOffsetSemitones` and `velocity` (pitch offset was already wired; velocity is the new bit)
- [x] PatternRenderer / WavWriter honour velocity in the offline render
- [x] `.beep` serializes velocity with a default-tolerant load (no schema bump needed — new field defaults cleanly for v1 files)

### Inspector panel (selected-event detail)
- [x] Thin inspector strip below the pattern grid; visible when an event is selected, hidden otherwise
- [x] Editable fields: start, duration, pitch offset (semitones), velocity, lane (dropdown)
- [x] Delete button
- [x] Each inspector edit pushes one undoable action (snapshot pattern → apply → perform)

### Visual clip differentiation
- [x] Clip height encodes velocity (vertically centred in the lane)
- [x] Selected clip gets a strong outline + small resize-handle dots at the edges
- [x] Hover lifts the clip's tint so the cursor target reads at a glance

### Clip editing gestures — the "feels like a DAW" pass
- [x] Cursor changes: crosshair over empty grid, open-hand over clip body, horizontal-resize over either edge
- [x] LEFT-edge resize (only the right edge resizes today)
- [x] Vertical drag inside a moving clip retargets the lane (snap to lane boundaries)
- [x] Click + drag on empty grid creates a clip whose duration matches the dragged distance (not the fixed 100 ms default)
- [x] Single click on empty grid (no drag) deselects — does NOT silently create a tiny clip
- [x] Double-click empty grid creates a clip at default duration (a discoverable alternative to drag-create)
- [x] Snap-target preview: a vertical guide line at the snap target while dragging or resizing
- [x] Arrow keys nudge selected event(s) by one grid step; shift+arrow nudges by ten
- [x] Esc deselects all events
- [x] Right-click a clip opens a context menu (Delete, Duplicate)

### Multi-select + clipboard
- [x] Shift-click a clip extends the selection
- [x] Cmd+A selects every event in the pattern
- [x] Cmd+C / Cmd+V duplicates the selected events to the playhead (relative timing + lane preserved)
- [x] Move / delete operations apply to the whole selection

### Lane usability
- [x] Editable lane-name field in the lane label column (replaces "1/2/3/4"; persisted in `.beep`)
- [x] Per-lane Mute toggle in the lane label column

### Empty-state hint
- [x] Update the "click in a lane to add a beep" hint to match the new gestures (mention drag-to-size and double-click-to-create)

## Phase 9 — Multi-voice patterns

Goal: each of the 4 lanes owns its own synth voice, so the user can layer different timbres in a single pattern (the canonical droid-chatter / R2D2 / "computer console" use case). The voice editor sections (Oscillator, Amp Env, Filter, Effects, Master) edit the currently-selected lane's voice.

### Architecture
- [x] `B33pProcessor` owns 4 `Voice` instances instead of 1
- [x] APVTS parameter IDs gain a lane prefix (e.g. `lane0_osc_waveform`); `ParameterIDs.h` reorganized per lane
- [x] Selected-lane state on the processor; the editor sections read/write the selected lane
- [x] Each event triggers its lane's voice; PatternRenderer mixes the four outputs
- [x] `.beep` schema bumped to v2; v1 → v2 migration fans the single voice's settings out to all 4 lanes' parameter sets

### UI
- [x] Click a lane label or any event in a lane to select that lane
- [x] Selected lane highlights visually (lane background tint)
- [x] Section titles append the lane name (e.g. "Oscillator (Lane 1)")
- [x] Per-lane Solo button complements Phase 8's Mute

### Workflow
- [x] "Copy lane settings to all" — fan out one voice's settings to all 4 lanes (Lane menu)
- [x] Per-lane "Reset voice to defaults" in a context menu (Lane menu)

---

## v0.2 polish & doc sweep

End-user review surfaced a stack of items that all amount to "the README and the build no longer agree, and the build itself has a few discoverability gaps that a fresh user would hit". Ordered by user-visible impact within each group.

### Documentation
- [x] **README sweep** — quickstart, shortcut table, features list, status badge, screenshot caption all reference UI that doesn't exist (Pre-alpha status, "big Dice at top", "Pattern tab", `Cmd+D` / `Cmd+E` shortcuts, musical-grid-snap promise, "Coming after v0.1" listing Custom Waveforms + Multi-voice). [major: turn-off-at-first-glance]
- [x] **Refresh hero screenshot** — current one predates the Lane menu, "(Lane N)" section suffixes, Mute/Solo per-lane buttons, Randomize buttons, Custom waveform. Caption references "(D)" / "(L)" letter buttons; they're icons now. [minor]
- [x] **`docs/USAGE.md`** — per-lane mental model, every keyboard shortcut, custom-waveform workflow, generate / clear lane via right-click, click-ruler-to-park-playhead. [minor]

### Missing user-facing features
- [x] **Audio settings menu** — JUCE `AudioDeviceSelectorComponent` in an Options menu. Without it, a user whose default output isn't usable has no in-app recovery. [major]
- [x] **Edit menu enrichment** — add Copy / Paste / Select All / Deselect entries that show their shortcuts (Cmd+C/V/A, Esc), plus an arrow-nudge note. Half the editing shortcuts are otherwise undiscoverable. [major]
- [x] **Generate / Clear lane in the Lane menu** — currently right-click only on empty lane area; both are otherwise invisible. [minor]
- [x] **Tiny output level meter in Master section** — user has no visual feedback that audio is happening. [minor]

### Confusing UX
- [x] **Explain "(Lane N)" in voice section titles** — fresh users have no in-app explanation that each lane has its own voice. Add a help blurb in About, or a one-line banner above the voice editor on first launch. [major]
- [x] **Pitch Envelope panel needs a "(shared across lanes)" suffix** — current asymmetry implies the curve is per-lane when it's actually shared. [minor]
- [x] **Visual cue when a Move drag hits the pattern wall** — currently silent clamp. [minor]
- [x] **Custom oscillator: explain the visual sine is a placeholder** — small footer note in the editor: "Drawing replaces the default sine cycle." [minor]

### Nice-to-have polish
- [x] **Per-lane background tint** in the voice editor sections so the "(Lane N)" suffix matches a visual cue and lane switching is obviously different.
- [x] **Recent Files submenu** in File.
- [x] **About dialog hyperlink to the GitHub repo.**

---

## Chores / tech debt

Cross-cutting work that isn't tied to any single phase. Review at the start of every session — if any item has a time-sensitive deadline approaching, flag it to the user before starting feature work.

- [x] Bump `actions/checkout` and `actions/cache` to `@v5` before June 2, 2026 (Node 20 deprecation — GitHub Actions warning surfaced on every run)

---

## Post-MVP roadmap

Ordered roughly by priority, not commitment. Each item becomes its own phase when prioritized.

### Sound-design power

- [x] **Custom drawn waveforms** — user draws a single-cycle oscillator shape (high-priority post-MVP) — shipped in v0.1.x
- [ ] **Wavetable oscillator** — morph between waveforms
- [ ] **FM oscillator** — two-operator FM with ratio + depth
- [ ] **Ring modulation** — second oscillator + ring-mod mix
- [ ] **Additional filters** — formant, comb, bandpass, highpass
- [ ] **Additional effects** — chorus, reverb, delay, flanger, phaser
- [ ] **LFOs and a modulation matrix** — any source → any destination
- [ ] **Per-event parameter overrides** beyond pitch (e.g., filter cutoff per hit)
- [ ] **Event probability, ratcheting, humanize**

### Visual feedback

- [x] **Filter response visualizer** — FabFilter/iZotope-style spectrum overlay showing the current cutoff + Q curve so the filter's behaviour reads at a glance; the log-skewed cutoff slider alone feels abrupt without the visual. (Coefficient smoothing was deferred — filter params currently push to the voice only on event trigger, so there's no live-drag zipper to fix yet. Revisit when modulation matrix or per-sample param updates land.)

### Musical timing / DAW integration

Explicitly out of MVP scope — the standalone app is the priority and the "music" framing comes later.

- [ ] **Tempo (BPM) + time signature** as pattern attributes
- [ ] **Musical snap values** (1/4, 1/8, 1/16, 1/32) derived from BPM
- [ ] **Dual time display** in the pattern ruler (bars/beats + seconds)
- [ ] **DAW transport sync** — when running as VST/AU, follow host tempo, play/stop, and bar position

### Workflow

- [ ] **Preset browser** with tags, favorites, search
- [ ] **Generator presets** — one-click starting points (droid chatter, alarms, weapon charge, UI beeps)
- [ ] **User-configurable randomization ranges and distributions**
- [ ] **MIDI input** for auditioning
- [ ] **Polyphony** for voices that benefit from it
- [ ] **Batch export** — render N dice-rolled variations in one shot

### Distribution

- [ ] **Release installers** — proper `.dmg` (macOS), Windows installer (NSIS / Inno Setup), Linux AppImage or tarball, auto-attached to a GitHub Release on tag push via CPack + a release workflow. Lives here rather than in the MVP because b33p is currently shared via "build from source" or "grab the CI workflow artifact"; installers come back when there's a public-facing release worth packaging.
- [ ] **VST3 plugin build**
- [ ] **AU plugin build** (macOS)
- [ ] **AAX plugin build** (Pro Tools)
- [ ] **Additional export formats** — FLAC, OGG, AIFF, 96 kHz, IMA ADPCM WAV (true 4-bit)
