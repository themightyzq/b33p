# Changelog

All notable changes to **b33p** are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) conventions, and
b33p adheres to [Semantic Versioning](https://semver.org/).

For the full per-commit history, see [`git log`](https://github.com/themightyzq/b33p/commits/main); for the active roadmap and per-task status, see [`TODO.md`](TODO.md).

## [Unreleased]

_Nothing yet. The 0.2.0 entry below was refreshed on 2026-05-26 to fold in this session's UI / state work (LookAndFeel, knob interactions, factory-preset restore, lane-voice clipboard, master-meter peak/clip, pitch-env axes, schema-version fix, off-screen-window fix) ahead of the v0.2.0 re-tag._

## [0.2.0] — 2026-05-26

The b33p 0.2.0 release. It was tagged feature-complete on 2026-04-30, then
refined through the 2026-05-25 design-review pass and plugin-hardening work
before release. Those refinements are listed first below; the original
feature-complete changelog follows under **"Original 0.2.0 feature set"**.

### Added

- `.gitattributes` declares cross-platform line-ending normalization (LF for text) and binary markers for `*.png`, `*.icns`, `*.ico`, `*.wav`.
- `docs/RELEASE_SMOKE.md` — per-OS smoke-test runbook for `v*` draft releases, covering install commands, host-DAW checks, the `auval` AU validation, and the publish / re-tag / fix-forward decision tree.
- `docs/REVIEW.md` — 2026-05-25 design review in two passes. Pass 1 (40 findings) audits the desktop-app surface: window layout, copy, empty states, keyboard. Pass 2 (38 findings) audits the plugin surface against the characterful-synth bar (u-he / Baby Audio / Soundtoys): host integration, knob craft, preset system, A/B compare, state recall, DSP-adjacent feedback. Together: 78 prioritized findings as the backlog source for future chore-list entries.
- `NOTICE` — third-party attribution for JUCE 8.0.12 (GPL-3.0) and Catch2 v3.14.0 (BSL-1.0).
- New `B33P_ENABLE_SANITIZERS` CMake option wires AddressSanitizer + UndefinedBehaviorSanitizer compile/link flags (GCC/Clang only). A dedicated `sanitizers` CI job on Ubuntu runs the full test suite under asan + ubsan on every push.
- `Help ▸ Keyboard Shortcuts...` opens a dedicated dialog with the keyboard reference grouped by Transport / File · Edit / Pattern editing — mirroring the README's table. The shortcut reference no longer needs to be hunted inside the About dialog.
- **File ▸ Restore Factory Presets...** rewrites the four shipped presets (FM Bell, Resonant Stab, Delay Pad, Ring Mod Robot) to their originals — recovering any you saved over — behind a confirmation. Your own presets are untouched. (REVIEW.md P22.)
- **Edit ▸ Lane ▸ Copy / Paste voice (clipboard)** — copy a dialed-in lane's whole voice (every parameter + its wavetable slots) to the system clipboard and paste it into another lane, or another b33p instance entirely. Paste is one undoable step. (REVIEW.md P24.)

### Changed

- Default editor width bumped from 900 to 1500 pt so the Pattern toolbar's full control set (Play / Loop / Follow / time / Length / Grid / BPM / Time-sig / Randomize All / Scope / Export) is visible without clipping. Hero screenshot refreshed against the new build.
- Pattern toolbar now wraps to a second row when the editor is laid out below 1440 pt — Length / Grid / BPM / Time-sig drop beneath the transport + actions clusters so every control stays reachable in narrow plugin-host panels. Wide-layout behaviour unchanged.
- Menu bar reorganized: the top-level `Lane` menu is demoted to `Edit ▸ Lane` (its five actions still scope to the currently-selected lane). The lane right-click menu on the pattern grid also gains Copy voice / Reset voice / Randomize All Lanes so the same actions are reachable from either surface.
- Destructive Lane menu actions — Copy voice to all lanes / Reset voice / Generate Random Pattern / Clear / Randomize All Lanes — now show a Cancel / Proceed confirmation dialog, matching the standard File ▸ New and File ▸ Open use for discarding unsaved work.
- Per-lane Mute / Solo buttons now differentiate state by luminance (dark off → bright on) and text contrast (dim grey off → white / dark on), not by color tint alone. Color-blind users see the state change via the brightness shift instead of relying on red vs yellow.
- Empty pattern hint is now two stacked lines instead of one cramped 137-character run-on — splits at the natural period so the first-run gesture-teach reads as instructions.
- Pitch Envelope shows a crosshair cursor on hover so it reads as an editable surface (was previously the default arrow). The zero-semitones baseline brightens when the curve is empty and gets a small "0 st" axis label so the empty canvas reads as a reference axis to draw against, not a passive display.
- Voice section titles no longer repeat `(Lane N)` six times — only the topmost section (Oscillator) carries the lane suffix as the single editor-wide lane indicator. Amp Envelope, Filter, Effects, Mod FX, Modulation, and Master drop the suffix.
- New `Help ▸ Audio Settings...` dialog explains where audio device settings live: the standalone wrapper's top-left Options gear icon in standalone, or host I/O routing under VST3 / AU. A user landing on a silent first launch now has a documented path to the recovery, even though the dialog itself doesn't yet open the wrapper's settings programmatically.
- Export dialog now defaults to 44.1 kHz instead of 48 kHz. Matches b33p's primary distribution context (music / web / games), where 44.1 kHz is the de-facto standard.
- Export dialog filename extension updates live when the user changes the format combo. Previously the rewrite happened only at submit time, so a user who picked FLAC mid-session still saw `b33p_export.wav` in the destination field until they clicked Export.
- Batch export of 10+ variations now goes through a Cancel / Render confirmation dialog so the user knows how many files are about to be written and that the UI will lock during the batch. Below 10 variations the render starts directly — fast enough that confirming would be friction.
- Parameter labels above every knob (`LabeledSlider`) now render bold instead of regular weight. Restores the param-name → value → control → randomizer hierarchy that the dice + lock buttons had been flattening.
- OscillatorSection and FilterSection cell width is capped at 180 pt with the cell group centered. Switching waveform mode (Sine → FM → Wavetable) or filter mode (Lowpass → Comb → Formant) no longer makes unchanged knobs like Pitch and Cutoff visibly resize to fill the new layout — they stay the same size regardless of how many siblings appear.
- Modulation section gains a small italic hint above the matrix rows: "Route a Source (LFO 1 or LFO 2) to a Destination, then dial the Amount to start modulating." Removes the "what do I do with this" cliff a first-time user used to face when opening the most powerful section in the editor.
- Mod FX slider labels now read "—" when the effect type is None (was misleadingly showing "Rate / Depth / Mix" from the Chorus placeholder). The dashes pair with the existing disabled-slider state to clearly signal "nothing meaningful here yet."
- Pattern grid snap-preview line is now visible at alpha 0.85 (was 0.55) while dragging clips. The line tells the user where their drag will land on release; the previous opacity was borderline invisible against the pattern background. The red wall-clamp variant stays distinguishable via color + thicker stroke.
- Pattern clip height now uses a `sqrt(velocity)` visual curve instead of linear. Low-velocity clips no longer collapse onto the 8 px floor at v ≤ 16% — the dead zone shrinks to v ≤ 2.5%, so the top-edge velocity drag stays useful through the bottom of the range. Audio behaviour is unchanged; this is a visual perceptual curve only.
- Lane name labels show an I-beam cursor on hover so the double-click rename affordance is discoverable without having to hover for the tooltip or read the docs.
- Master Gain knob now reads in dB (e.g. `0.0 dB` at unity, `+6.0 dB` at 2.0 linear) instead of the raw linear value. Audio users speak dB; typing `+3 dB` parses back to the right linear value.
- Save-Preset failure dialog is now actionable: it names the invalid characters to avoid (`/ \ : * ? " |`) and prints the full on-disk path to the presets folder so the user can fix permissions if that's the cause.
- Preset browser shows an empty-state hint ("No presets yet. Use File ▸ Save Preset…") when the directory is empty, and renders factory presets (those whose name starts with `Factory - `) in italic with a slightly cooler tint so they're visually distinct from user-saved presets at a glance.
- File ▸ Open Recent submenu stays enabled when the MRU list is empty and shows a disabled "No recent files" placeholder inside. Was previously a greyed-out parent submenu with no children, which read like a broken feature instead of an inactive one.
- Per-section accent strip is now 4 px at full alpha (was 2 px at α 0.85). The lane-identity color carries across the editor at normal viewing distance instead of being almost invisible.
- WaveformEditor footer is now two stacked lines instead of one run-on. Line 1 names the gesture; Line 2 explains Custom vs Wavetable slot semantics. Easier to scan.
- Pattern ruler bar labels now skip in powers of two when bars get too close to read (high BPM + narrow grid). No change at standard 120 BPM / 4/4 / wide grid; just kicks in when labels would collide.
- Pattern ruler tick hierarchy is now visually separable at a glance: grid lines dim further (RGB 40 → 32), bar ticks pop (150 → 200), beats stay where they were. The grid → beat → bar layers no longer blur into one another.
- Inspector strip in multi-select mode now lists the applicable shortcuts (Delete, arrow-key nudge, Cmd+C / Cmd+V) instead of just the count, so users know what they can do with the selection.
- Pattern grid lanes now show a subtle hover tint (alpha 0.07) when the cursor is over them — sitting visually between the default 0.04 and the selected-lane 0.10. Tells the user which lane will receive a click / drag-create before they commit.
- Parked playhead is now bright (230,230,240) instead of dim (140,140,140), so it sits above the brighter bar-tick layer. The playhead is the paste-anchor for Cmd+V — it should be the most prominent vertical line in the pattern, not blend into the ruler.
- Inspector empty-state copy now names what the inspector edits ("Select a clip to edit lane, start, duration, pitch, and velocity.") instead of the previous "Click an event to edit it.", so first-time users know there's something worth selecting an event for.
- About dialog's "Keys" section now points at Help ▸ Keyboard Shortcuts… instead of duplicating the shortcut table. Single source of truth in the dedicated dialog; no drift when shortcuts change.
- Unsaved-changes prompt now names the file ("Save changes to MyPatch.beep before closing?") instead of the abstract "This project has unsaved changes. Save before closing?" — the filename is what the user is mentally tracking.
- Host bypass is now honored — when the DAW engages bypass (button or automation), b33p outputs silence instead of letting voices play through. New `host_bypass` APVTS parameter is exposed via `getBypassParameter()` so VST3 / AU hosts route their bypass automation to it; processBlock checks the value and early-returns with a cleared buffer.
- Latency reporting wired — `setLatencySamples(0)` is called explicitly in `prepareToPlay` so the host's plugin delay compensation contract is in place even though we don't introduce latency today.
- Tail length reporting — `getTailLengthSeconds()` now returns 4 seconds (was 0). Covers worst-case audible tail from amp-envelope release + reverb decay + delay feedback so DAWs don't cut off reverb wash or delay echoes mid-decay when transport stops.
- **A/B compare** — top-right of the Master section gets three small buttons: `A` / `B` toggle + `Copy`. Snapshots store the APVTS parameter state only (not Pattern, pitch curve, or wavetables — A/B is for comparing patches, not songs). First switch to B copies A's current state into B so the user starts with something to tweak, not an init patch. Both snapshots + the active slot persist in the `.beep` file and DAW project state (schema bump v12 → v13).
- **In-plugin Undo / Redo buttons** — top-left of the Master section. Gives a click-path that survives the DAW capturing the Cmd+Z keyboard shortcut. Enable/disable state matches `UndoManager::canUndo()` / `canRedo()`.
- **Master level meter** gains a peak-hold marker (a bright tick that holds the recent maximum ~1.5 s, then eases down) and a latching **clip indicator** (a red cap at the right end that lights for ~1.5 s whenever the output hits 0 dBFS, so a brief overload isn't missed between frames). (REVIEW.md P18.)
- **Pitch Envelope** canvas now shows axis reference: faint vertical gridlines at the quarter points of the note duration and `+12 / 0 / -12` semitone labels down the left edge, so the curve reads against concrete pitch values. (REVIEW.md P32.)
- The **selected lane** now persists in saved projects and DAW sessions, and the **editor window size** persists across DAW close / reopen (stored in the plugin's DAW state, deliberately *not* in shared `.beep` files — window size isn't portable patch data). (REVIEW.md P7 / P11.)
- Numeric value readouts under each knob (and the drag-time popup bubble) now use a **monospaced** face, so digits no longer shift left/right as a value ticks during a drag. (REVIEW.md P28.)
- VST3 / AU plugin **subcategory** is now declared explicitly (`Instrument | Synth` / `kAudioUnitType_MusicDevice`) so host plugin pickers that filter by subcategory list b33p correctly. (REVIEW.md P27.)
- **Knob interactions** (REVIEW.md P9 / P10 / P25 / P36). Every rotary now supports the synth standards: hold **Shift** before dragging for fine adjustment, **mouse-wheel / two-finger scroll** to step the value, **double-click** to reset to the parameter default, and a **right-click menu** (Enter value… / Reset to default) — so typing an exact value is finally discoverable.
- Custom flat dark **LookAndFeel** (REVIEW.md P8 / P15 / P29 / P30). Rotary knobs drop JUCE's default line indicator for a flat body + thin value arc + dot, and the arc takes the **selected lane's accent colour** (blue / green / amber / pink) so the knobs shift colour with the section strips when you switch lanes. Comboboxes are flat with a hairline border + accent caret; buttons are flat rounded; popup menus and dialogs match the charcoal palette. Modulation-amount sliders now render bipolar (fill grows from the centre). Installed process-wide so the editor and its dialogs share one look.

### Removed

- `Cmd+/` keyboard shortcut for the About dialog — it was non-standard (Cmd+/ is comment-toggle in most editors) and self-defeating (the shortcut was only documented inside the dialog it opens). About stays reachable via Help menu.

### Fixed

- Closing the standalone window or quitting it (Cmd+Q / the OS quit) with unsaved changes now prompts **Save / Discard / Cancel** instead of silently discarding the project. The standalone build had lost this when the target switched to `juce_add_plugin` — JUCE's default `StandaloneFilterApp` wrapper has no dirty check. b33p now ships a custom standalone app (`JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP`, `Source/StandaloneApp.cpp`) that routes both the window-close button and Cmd+Q through the same confirmation File ▸ New / Open already use. Plugin (VST3 / AU) builds are unaffected — the host owns the window lifecycle there.
- `Build and Launch.command` and `build_and_launch.py` now reconfigure the CMake build directory when its cached `CMAKE_BUILD_TYPE` differs from the requested one, and only pass `-G <generator>` on a *fresh* configure (omitting it on reconfigure so CMake reuses the cached generator). Two interacting bugs were biting in sequence: single-config generators (Make, Ninja) silently ignore `cmake --build --config <X>` so switching between Debug and Release used to build the cached type and then fail to find the artefact; and once that was fixed, passing `-G` on reconfigure tripped CMake's "generator does not match the generator used previously" check when ninja showed up on PATH after the first configure.
- The `.beep` / plugin-state schema version is now correctly stamped as **13** — it had lagged at 12 while the saved content already included v13 A/B-compare data. Files written by earlier 0.2.0 builds (stamped 12) still load and migrate forward.
- Loading a preset is now a single undoable step — Cmd+Z (or the in-plugin Undo button) restores the previous patch instead of losing it. (REVIEW.md P6.)
- Saving a preset under a name that already exists now asks for confirmation before overwriting, instead of silently replacing the existing preset. (REVIEW.md P26.)
- Standalone window now fits the screen on launch. The voice editor was a six-row vertical stack ~1338 px tall — taller than a 1080p display's usable height — so the pattern grid opened off the bottom of the screen. The three full-width-but-short rows (Mod FX, Modulation, Pitch Env) are now column-packed: Mod FX sits beside Effects + Master, and Modulation sits beside Pitch Env, bringing the default window to ~1007 px. The standalone window also clamps to the display's usable area on launch (and stays resizable), so it never opens partly off-screen. The editor additionally gained resize limits (min 1000×600, max 3200×2200) so it can't be dragged down to an unreadable sliver or ballooned past sane bounds (REVIEW.md P12).

### Original 0.2.0 feature set (feature-complete tag, 2026-04-30)

A 28-commit session on 2026-04-29 worked top-to-bottom through the post-MVP
roadmap, then closed it out. The synth grew from "monophonic 4-lane sketchpad"
into a full-featured instrument with VST3 / AU plugin builds, host transport
sync, a modulation engine, presets, MIDI input, polyphonic playback, and a
multi-format renderer. Final release-prep work on 2026-04-30 added the
release workflow, made the macOS build Universal, regenerated the .icns at
the full Apple-recommended size set, and brought the docs in line with what
ships.

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

[Unreleased]: https://github.com/themightyzq/b33p/compare/v0.2.0...HEAD
[0.2.0]: https://github.com/themightyzq/b33p/releases/tag/v0.2.0
[0.1.0]: https://github.com/themightyzq/b33p/releases/tag/v0.1.0
