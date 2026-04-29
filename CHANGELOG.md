# Changelog

All notable changes to **b33p** are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) conventions, and
b33p adheres to [Semantic Versioning](https://semver.org/).

For the full per-commit history, see [`git log`](https://github.com/themightyzq/b33p/commits/main); for the active roadmap and per-task status, see [`TODO.md`](TODO.md).

## [Unreleased] — post-v0.1.0 development

A 28-commit session on 2026-04-29 worked top-to-bottom through the post-MVP
roadmap, then closed it out. The synth grew from "monophonic 4-lane sketchpad"
into a full-featured instrument with VST3 / AU plugin builds, host transport
sync, a modulation engine, presets, MIDI input, polyphonic playback, and a
multi-format renderer.

CMake project version bumped from `0.0.1` to `0.2.0` to reflect the scale of
post-v0.1.0 work — propagates into the plugin manifest, About dialog, and
`B33P_VERSION_STRING`.

### Added — sound design

- **Wavetable oscillator** — 4 morphable single-cycle slots per lane, blended by
  a continuous 0..1 morph parameter. Slot drawing reuses the Custom oscillator's
  canvas with four slot tabs.
- **FM oscillator** — two-operator sine FM with ratio (0.1..16) and depth (0..10)
  parameters. Depth = 0 collapses to a clean carrier sine.
- **Ring modulator oscillator** — sine carrier × sine modulator with wet/dry
  crossfade. Mix = 0 collapses to a clean carrier sine.
- **Multi-mode filter** — Lowpass / Highpass / Bandpass (12 dB/oct biquad),
  feedback Comb, Formant (5 vowels A/E/I/O/U interpolated). Cutoff / resonance /
  vowel sliders reflow per type.
- **Filter response visualiser** — log-frequency × dB plot above the filter
  controls; analytic response math switches by filter type, tracks slider drags
  live.
- **Modulation effect slot** — Chorus / Reverb / Delay / Flanger / Phaser at the
  end of the per-voice chain. Three normalised parameters carry type-dependent
  semantics; slider labels update per type.
- **LFOs and modulation matrix** — 2 free-running LFOs per lane (Sine / Triangle
  / Saw / Square, 0..30 Hz), 4 matrix slots routing either LFO to one of 11 voice
  destinations with a bipolar amount.
- **Per-event parameter overrides** — every pattern event can pin up to 4 voice
  parameters for that hit only. Right-click clip → *Edit overrides…*.
- **Per-event probability / ratcheting / humanize** — three new fields per event,
  rolled / expanded / jittered at snapshot time.

### Added — pattern + playback

- **Tempo + time signature** — 20..999 BPM, 9 common signatures (4/4 through
  6/4). Drives both the snap grid and the dual-time ruler.
- **Musical snap values** — 1/32 note through Whole, computed from the current
  BPM. Lives alongside the existing time-based snap entries.
- **Dual-time ruler + playhead readout** — seconds along the top, bar / beat
  ticks along the bottom. Playhead reads `Bar 2.3 — 0.95 / 5.00s`.
- **DAW transport sync** — opt-in "Follow" toggle in PatternSection. When on
  (and a host playhead is available), pattern playback mirrors host play /
  stop and snaps the playhead to `host_time mod pattern_length` at the top of
  every block. Pattern BPM stays independent — sound-design users routinely
  want their pattern at a different tempo than the host session.

### Added — workflow

- **Preset browser** — `File ▸ Save Preset...` and `File ▸ Browse Presets...`
  menu items, per-user presets directory, ListBox with Load / Delete.
- **Factory presets** — 4 starter patches seed the presets directory on first
  launch (FM Bell, Resonant Stab, Delay Pad, Ring Mod Robot).
- **Randomization scope slider** — 0.05..1.0 multiplier on every dice roll, in
  the pattern controls row. 1.0 keeps the legacy full-range behaviour; smaller
  values constrain rolls to a window around the current value.
- **MIDI input** — every connected input device routes notes to the selected
  lane. Note 60 = no transposition; velocity-sensitive.
- **Polyphony** — 8-voice MIDI keyboard pool alongside the existing per-lane
  pattern voices, so chords ring together up to 8 notes deep.
- **Batch export** — Variations field in the Export dialog (1..100). Variation
  1 captures the patch verbatim; subsequent variations re-roll all unlocked
  parameters. APVTS state is snapshotted and restored around the batch.

### Added — distribution

- **VST3 + AU plugin builds** — single CMake target via `juce_add_plugin`
  produces `b33p.vst3` and `b33p.component` (macOS) alongside the standalone
  `b33p.app`.
- **Additional audio export formats** — AIFF / FLAC / OGG Vorbis added to the
  Export dialog; 88.2 kHz and 96 kHz added to the sample-rate options. The
  destination filename's extension auto-rewrites to match the chosen format.

### Added — visual polish

- **Per-lane accent colour system** — every lane has a stable accent (blue /
  green / amber / pink). Voice editor sections paint a 2 px accent strip below
  their title bar; pattern grid tints every lane row with the same palette.
- **Recent Files submenu** — `File ▸ Open Recent` lists the 10 most-recently
  opened or saved files, persisted via `juce::PropertiesFile`.
- **Filter response visualiser** — see Sound Design section above.

### Changed

- **`.beep` file format bumped from v2 to v12** with explicit forward-only
  migrations for every step (v2 → v3 wavetable, v3 → v4 FM, v4 → v5 Ring,
  v5 → v6 multi-mode filter, v6 → v7 mod-FX, v7 → v8 LFO + matrix, v8 → v9
  per-event overrides, v9 → v10 probability / ratchets / humanize, v10 → v11
  BPM + time signature, v11 → v12 follow-host-transport flag). Every prior
  version still loads — no breaking migrations.
- **Build target switched from `juce_add_gui_app` to `juce_add_plugin`.** The
  standalone `b33p.app` is now produced by JUCE's `StandaloneFilterApp` wrapper
  alongside the VST3 / AU bundles. The custom Application class with single-
  instance enforcement, quit-confirm, and command-line file-open routing was
  removed in the process — restoring those features is a follow-up commit
  behind a custom standalone wrapper.
- **`Source/Pattern/WavWriter.{h,cpp}`** renamed to `AudioFileWriter.{h,cpp}`
  to reflect the multi-format dispatch. The original `writeWav` signature is
  preserved as an inline alias.
- **Window grew by 4 rows** to accommodate the new Mod FX, Modulation,
  modulation-matrix, and BPM/time-sig controls.
- **`actions/checkout` and `actions/cache`** bumped to `@v5` ahead of the
  Node 20 deprecation deadline.
- **Custom oscillator's storage** is now slot 0 of the wavetable storage
  (Custom and Wavetable modes share the same per-lane data).
- **About dialog** gains an "Open GitHub Page" button.
- **OscillatorSection's bottom row** uses a conditional layout — only the
  mode-relevant sliders are visible. Adding a new oscillator mode is now
  purely additive.

### Removed

- **`Source/UI/AudioSettingsWindow.{h,cpp}`** — JUCE's StandaloneFilterApp
  wrapper provides its own audio device picker via the Options gear icon in
  the title bar. The custom Audio Settings menu item was removed; the host
  picker is the replacement.
- **`Source/Main.cpp` shrank** from 145 lines (custom Application class) to a
  4-line `createPluginFilter` factory. JUCE's plugin client wrappers replace
  the previous custom event-handling code.
- **AAX plugin format** dropped from the post-MVP roadmap — gated behind the
  (non-free) Avid SDK and serving an audience too narrow for an indie sound-
  design tool. `juce_add_plugin` is unchanged; AAX can be added later behind
  an opt-in CMake option if the SDK ever becomes available.
- **IMA-ADPCM WAV export** dropped from the post-MVP roadmap — JUCE doesn't
  ship a writer and the format's audience (legacy game-console pipelines)
  doesn't justify a from-scratch encoder.
- **Release installers** dropped from the post-MVP roadmap — binaries are
  distributed via the existing CI artifacts on each green push to `main`,
  which is plenty for the audience b33p targets. CPack + signed `.dmg` /
  installer pipelines aren't planned.

### Deferred regressions

These features were lost when the build target switched from `juce_add_gui_app`
to `juce_add_plugin`. JUCE's `StandaloneFilterApp` wrapper produces the
standalone `b33p.app` now and replaces our custom Application class. Restoring
each one means subclassing JUCE's wrapper via
`JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP` and carrying the original behaviour
into the override.

- **Single-instance enforcement** — second copies of the standalone .app no
  longer route to the running instance; each launches independently.
- **Quit-confirm-on-dirty** — closing the standalone window with unsaved
  changes no longer prompts Save / Discard / Cancel. Quit-prompt for the
  in-app File ▸ New / File ▸ Open paths still works (those run inside
  MainComponent, not the wrapper).
- **Command-line `.beep` file open** — double-clicking a `.beep` from Finder
  / dragging onto the dock no longer routes the file to the running standalone
  instance.

### Fixed

- **`-Wshadow-uncaptured-local` warning** in `ProjectFileManager::saveAs`'s
  `FileChooser` lambda — captured `onComplete` was shadowing the outer
  parameter.
- **OscillatorSection layout shifting on mode change** — adding the conditional
  layout changed the calculus, but the user-visible behaviour now is "only the
  relevant sliders appear", which is the right call for >= 2 modes.

## [0.1.0] — 2026-04-26

The initial public release. b33p shipped as a standalone macOS / Windows /
Linux app with 4-lane patterns, drawable pitch envelope, per-lane voice
design (Sine / Square / Triangle / Saw / Noise / Custom oscillator), ADSR,
LP filter, bitcrush + distortion, master gain, and WAV export. See the
git history before the v0.1.0 tag for the per-feature commits.

[Unreleased]: https://github.com/themightyzq/b33p/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/themightyzq/b33p/releases/tag/v0.1.0
