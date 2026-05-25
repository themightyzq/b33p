# TODO.md

b33p roadmap. Each phase is a shippable milestone. Pick tasks **only from the current phase**. Check items off when done; do not delete them.

> **Current focus: v0.2.0 release is staged as a *draft pre-release* on GitHub. Publishing is gated on per-OS smoke tests of the artefact bundles attached to that draft (see "Release in flight" below). After publish, work resumes on the chores list.**

---

## Release in flight — v0.2.0

**Status:** Tag `v0.2.0` is on the remote (commit `6876f4b`). The release workflow built and attached three archives to a **draft pre-release** at <https://github.com/themightyzq/b33p/releases/tag/v0.2.0>. **Publish has not happened yet.**

### Smoke tests required before clicking Publish

For each archive (download from the Releases page, **not** from a local build):

- [ ] **`b33p-0.2.0-macos-universal.zip`** — extract; right-click → Open (or `xattr -dr com.apple.quarantine /path/to/b33p.app`); confirm the standalone launches, the audition button works, a factory preset loads, a quick WAV exports. Drop `b33p.vst3` into `~/Library/Audio/Plug-Ins/VST3/` and `b33p.component` into `~/Library/Audio/Plug-Ins/Components/`; confirm both load in Logic / Ableton / Reaper. `auval -v aumu B33p Zqsf` should pass for the AU.
- [ ] **`b33p-0.2.0-windows-x64.zip`** — extract; SmartScreen → More info → Run anyway; standalone launches, audition + render. VST3 install via host's plugin path; DAW scan picks it up.
- [ ] **`b33p-0.2.0-linux-x86_64.tar.gz`** — extract; `chmod +x b33p`; standalone launches, audition + render. VST3 install via host's plugin path.

### Decision after smoke tests

- **All three pass:** click **Publish release** on the draft page (the pre-1.0 pre-release flag is correct — leave it on). Then link / announce externally.
- **Any fail:** **don't publish.** Two paths:
  1. **Re-tag**: `gh release delete v0.2.0 --yes`, `git tag -d v0.2.0 && git push origin :refs/tags/v0.2.0`, fix on `main`, re-tag `v0.2.0`. Acceptable because nothing is published yet.
  2. **Fix forward as v0.2.1**: leave v0.2.0 published as broken, ship a quick patch tag.
  Re-tag is cleaner pre-publish; fix-forward is right post-publish.

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
- [x] **Audio settings menu** — delivered via JUCE's standalone wrapper (top-right gear icon → Audio Settings opens an `AudioDeviceSelectorComponent`); `MainComponent` stays decoupled from the device layer. Standalone-only by design; under VST3 / AU the host manages the device. [major]
- [x] **Edit menu enrichment** — add Copy / Paste / Select All / Deselect entries that show their shortcuts (Cmd+C/V/A, Esc), plus an arrow-nudge note. Half the editing shortcuts are otherwise undiscoverable. [major]
- [x] **Generate / Clear lane in the Lane menu** — currently right-click only on empty lane area; both are otherwise invisible. [minor]
- [x] **Tiny output level meter in Master section** — user has no visual feedback that audio is happening. [minor]

### Confusing UX
- [x] **Explain "(Lane N)" in voice section titles** — fresh users have no in-app explanation that each lane has its own voice. Add a help blurb in About, or a one-line banner above the voice editor on first launch. [major]
- [x] **Pitch Envelope panel needs a "(shared across lanes)" suffix** — current asymmetry implies the curve is per-lane when it's actually shared. [minor]
- [x] **Visual cue when a Move drag hits the pattern wall** — currently silent clamp. [minor]
- [x] **Custom oscillator: explain the visual sine is a placeholder** — shipped as a footer in the WaveformEditor: "Click + drag to draw. Custom mode plays Slot 1; Wavetable mode blends all four via the Morph slider." [minor]

### Nice-to-have polish
- [x] **Per-lane background tint** in the voice editor sections so the "(Lane N)" suffix matches a visual cue and lane switching is obviously different.
- [x] **Recent Files submenu** in File.
- [x] **About dialog hyperlink to the GitHub repo.**

---

## Chores / tech debt

Cross-cutting work that isn't tied to any single phase. Review at the start of every session — if any item has a time-sensitive deadline approaching, flag it to the user before starting feature work.

- [x] Bump `actions/checkout` and `actions/cache` to `@v5` before June 2, 2026 (Node 20 deprecation — GitHub Actions warning surfaced on every run)
- [x] Add `.gitattributes` (`* text=auto eol=lf` plus binary marks for `*.png`, `*.icns`, `*.ico`, `*.wav`).
- [x] Add `docs/RELEASE_SMOKE.md` — per-OS smoke-test runbook + failure-mode triage + publish / re-tag / fix-forward decision tree for any `v*` draft release.
- [x] Capture the 2026-05-25 design review in `docs/REVIEW.md` — 40 prioritized UX findings (10 Critical / 19 High Impact / 11 Nice to Have). New chore-list entries get extracted from this as they're worked.
- [x] Widen the MainComponent default to 1500 pt so the Pattern toolbar (Play / Loop / Follow / time / Length / Grid / BPM / Time-sig / Randomize / Scope / Export) fits without clipping. Refresh `docs/images/hero.png` against the wider build. (REVIEW.md Critical #1; the visible-clip half.)
- [x] Wrap the Pattern toolbar at narrow widths so plugin hosts with constrained UI panels don't lose access to BPM / Time-sig / Length / Grid. Wide-width fit is in; narrow-width graceful degradation is still TODO. (REVIEW.md Critical #1, remaining half.) — Below 1440 pt the settings cluster (Length / Grid / BPM / Time-sig) drops to a second toolbar row. Verified at 1500 (single row) and 1200 (wrapped) widths.
- [x] Add `NOTICE` crediting JUCE 8.0.12 (GPL-3) and Catch2 v3.14.0 (BSL-1.0).
- [ ] Set the GitHub repository's social preview image (Settings → Social preview).
- [x] Wire **sanitizer** CI rails — `B33P_ENABLE_SANITIZERS` CMake option enables `-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer` on GCC/Clang; new `sanitizers` job on Ubuntu builds B33pTests under asan + ubsan with fail-fast halt-on-error. Apple Universal builds warn-and-skip (multi-arch + asan is incompatible). **clang-tidy** stays a follow-up.
- [ ] Wire **clang-tidy** static-analysis CI rail — `.clang-tidy` config + GitHub Actions job. Deferred from the sanitizer-CI pass because a first run on a v0.2.0 codebase that has never been clang-tidied will surface dozens of triage items; scoping it separately keeps that effort bounded.
- [x] Demote the top-level `Lane` menu — move its five items to `Edit ▸ Lane`, drop `Lane` from the menu bar (`File · Edit · Help`), and extend the lane right-click context with Copy voice / Reset voice / Randomize all so it carries the same actions. (REVIEW.md Pass 1 Critical #3.)
- [x] Drop `Cmd+/` shortcut for About — the binding is non-standard (Cmd+/ is comment-toggle in most editors) and self-defeating (shortcut was only documented inside the dialog it shortcut). About stays reachable via Help menu. (REVIEW.md Pass 1 Critical #4.)
- [x] Add `Help ▸ Keyboard Shortcuts...` entry — dedicated dialog mirroring README's keyboard table (Transport / File · Edit / Pattern editing). The shortcut reference is no longer buried inside About. (REVIEW.md Pass 1 Critical #5.)
- [x] Confirm destructive Lane menu actions — Copy voice / Reset voice / Generate / Clear / Randomize All now route through a Cancel-or-Proceed dialog (the same standard File ▸ New / Open already use via `confirmDiscardThen`). PatternGrid lane right-click still skips the confirm; matching the menu surface is a follow-up. (REVIEW.md Pass 1 Critical #6.)
- [x] Mute / Solo lane buttons now differentiate by luminance + text contrast — off state is dark with dim grey text; on state is bright fill (red / yellow) with high-contrast text. Color-blind users get the state via brightness shift, not red vs yellow. (REVIEW.md Pass 1 Critical #7.)
- [x] Empty pattern hint split into two stacked lines instead of one 137-character run-on. Splits at the natural period so the first-run gesture-teach reads as instructions, not a wall of text. (REVIEW.md Pass 1 Critical #8 — minimum sufficient fix; ghost-clip onboarding flourish stays a follow-up.)
- [x] Pitch Envelope empty state — crosshair cursor on hover signals editability, the 0-semitones baseline brightens (and gets a "0 st" axis label) so it reads as a reference axis. Hint rephrased to "Click anywhere to add a pitch point." (REVIEW.md Pass 1 Critical #9.)
- [x] Drop the repeated `(Lane N)` suffix from six of the seven voice sections — Oscillator (the topmost) keeps it as the single lane indicator. AmpEnv / Filter / Effects / Mod FX / Modulation / Master no longer repeat the same context six times. The full lane-header-strip enhancement REVIEW.md prescribed stays a follow-up. (REVIEW.md Pass 1 Critical #10.)
- [x] Refresh two stale `v0.2 polish & doc sweep` entries (Audio settings menu, custom-waveform footer) to match what actually shipped. Wording-only fix; the code was already correct on both. (REVIEW.md Pass 1 HI #11.)
- [x] Help ▸ Audio Settings... dialog — points the user at the standalone wrapper's gear icon (standalone path) or the DAW's I/O routing (plugin path). Documentation-as-recovery; programmatic launch of the wrapper dialog requires custom-standalone-app plumbing tied to the deferred-regressions block. (REVIEW.md Pass 1 HI #12.)
- [x] Export dialog default sample rate is now 44.1 kHz (was 48 kHz). Matches b33p's music / web / games distribution context; the 8 / 11.025 / 16 / 22.05 kHz lo-fi options ahead of it stay as deliberate retro choices. (REVIEW.md Pass 1 HI #13.)
- [x] Export dialog filename extension now updates live when the user changes the format combo (was previously only rewritten at submit-time, so a user picking FLAC would still see `…wav` in the destination field until they clicked Export). (REVIEW.md Pass 1 HI #14.)
- [x] Export pre-flight confirm at variations ≥ 10 — Cancel/Render dialog tells the user how many files are about to render, that the UI locks during the batch, and gives them a chance to back out. Below 10, batch is fast enough that the confirm would be friction. Time-estimate + in-progress cancel stay follow-ups (would need calibrated render timing + async task plumbing). (REVIEW.md Pass 1 HI #15.)
- [x] REVIEW.md Pass 1 HI #16 (ExportTask blocks UI thread) — false positive, verified via `ExportTask.h:28` (already a `juce::ThreadWithProgressWindow` with modal progress dialog + step-boundary cancellation). No code change. Mid-render cancellation is intentionally deferred per the existing design comment.
- [x] REVIEW.md Pass 1 HI #17 (AmpEnvelopeVisualizer doesn't animate during drag) — false positive, verified via `AmpEnvelopeVisualizer.cpp:46-49` (uses `apvts.addParameterListener` which fires `parameterChanged` on every slider tick during drag). No code change.
- [x] LabeledSlider parameter label is now bold (was regular 11 pt) — gives the knob name visual weight comparable to or stronger than the dice + lock buttons below the slider, restoring the param-name → value → control → randomizer hierarchy. (REVIEW.md Pass 1 HI #18.)
- [x] OscillatorSection + FilterSection cell width is now capped at 180 pt with the cell group centered. Switching between modes (Sine → FM → Wavetable, or Lowpass → Formant) no longer makes unchanged knobs visibly resize — Pitch / Cutoff / Resonance stay the same size whatever the mode reveals. Tight modes (3+ visible sliders) fall back to the previous proportional layout. (REVIEW.md Pass 1 HI #19.)
- [x] ModulationSection has a small italic hint line above the matrix rows: "Route a Source (LFO 1 or LFO 2) to a Destination, then dial the Amount to start modulating." Always visible — short enough to be quiet in full flight, prominent enough that a first-time user has a starting point. (REVIEW.md Pass 1 HI #20.)
- [x] ModEffects slider labels read "—" when type=None (was misleading "Rate / Depth / Mix" from the Chorus placeholder). Pairs with the existing disabled state to say "nothing meaningful here yet." (REVIEW.md Pass 1 HI #21.)
- [x] Snap-target preview line alpha bumped from 0.55 → 0.85 so the valid-snap guide is reliably visible during drag. Wall-clamp variant (red, thicker stroke) stays distinguishable. (REVIEW.md Pass 1 HI #22.)
- [x] Velocity → clip-height mapping switched from linear to `sqrt(v)`. Low velocities now render with visible differentiation (dead zone where the 8 px floor kicks in shrinks from v≤16% to v≤2.5%). Audio behaviour unchanged. (REVIEW.md Pass 1 HI #23.)
- [x] Lane name labels show an I-beam cursor on hover, signalling the double-click rename affordance that was previously discoverable only by reading the tooltip or the docs. (REVIEW.md Pass 1 HI #24.)
- [x] Master Gain knob now displays dB (e.g., `+6.0 dB` for the 2.0 linear value) instead of raw linear `1.00`. Round-trips: typed `+3 dB` parses back to 10^(3/20) ≈ 1.41 linear. New `SliderFormatting::applyLinearGainAsDb` helper. (REVIEW.md Pass 1 HI #25.)
- [x] Save-Preset failure message is now actionable — names the invalid characters to avoid and shows the on-disk presets path so the user can check permissions. (REVIEW.md Pass 1 HI #26.)
- [x] Preset browser empty state ("No presets yet. Use File ▸ Save Preset…") + factory presets render italic + slightly cooler tint so they're visually distinct from user-saved presets at a glance. Restore-factory-presets action stays a follow-up. (REVIEW.md Pass 1 HI #27.)
- [x] REVIEW.md Pass 1 HI #28 (plugin vs standalone title identical) — false positive, verified at `MainComponent.cpp:331-333`. `updateWindowTitle()` returns early when no `juce::DocumentWindow` ancestor exists, which is exactly the plugin case — so the host's own title (DAW track + plugin name) remains. Standalone shows `b33p - <file>`. Titles are NOT identical between modes. No code change.
- [x] Open Recent submenu now stays enabled when the MRU is empty and shows a disabled "No recent files" placeholder inside, instead of disabling the whole submenu (which read as "feature broken"). (REVIEW.md Pass 1 HI #29.)
- [x] Section accent strip bumped from 2 px / α 0.85 to 4 px / α 1.0 so the per-lane color carries across the editor without becoming heavy. (REVIEW.md Pass 1 NTH #30.)
- [x] WaveformEditor footer split into two stacked lines (was a single run-on sentence). Line 1 = gesture, Line 2 = slot semantics. (REVIEW.md Pass 1 NTH #31.)
- [x] REVIEW.md Pass 1 NTH #32 (slot numbering 1-vs-0 cosmetic) — no action needed. REVIEW.md itself notes this is the "friendly choice" (UI is 1-indexed, code is 0-indexed) and was logged for awareness rather than action. Standard convention; no fix.
- [x] Pattern ruler bar labels now use power-of-two skip (1, 2, 4, …) when adjacent labels would overlap. At standard 120 BPM / 4/4 / wide grid → no skip; high BPM in narrow plugin hosts → labels thin out until they fit. (REVIEW.md Pass 1 NTH #33.)
- [x] Ruler tick hierarchy widened: grid lines 40 → 32 (recede further), bar ticks 150 → 200 (dominate). The grid → beat → bar layer separation now reads pre-attentively instead of all blurring together. (REVIEW.md Pass 1 NTH #34.)
- [x] Multi-select inspector now tells the user which shortcuts apply to the selection: "N events selected · Delete · ←/→ nudge · Cmd+C / Cmd+V" (was just "N events selected."). (REVIEW.md Pass 1 NTH #35.)
- [ ] Add `.github/ISSUE_TEMPLATE/` and `PULL_REQUEST_TEMPLATE.md`.

---

## Deferred regressions

Features that were lost when the build target switched from `juce_add_gui_app` to `juce_add_plugin`. JUCE's `StandaloneFilterApp` wrapper produces the standalone `b33p.app` now and replaced our custom Application class; restoring each one means subclassing the wrapper via `JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP` and carrying the original behaviour into the override.

- [ ] **Single-instance enforcement** — second copies of the standalone .app no longer route to the running instance.
- [ ] **Quit-confirm-on-dirty for the standalone window** — closing the wrapper window with unsaved changes doesn't prompt. (File ▸ New / Open paths still confirm — those run inside MainComponent, not the wrapper.)
- [ ] **Command-line `.beep` file open** — double-clicking a `.beep` in Finder / dragging onto the dock no longer routes into the running standalone instance.

---

## Post-MVP roadmap

Ordered roughly by priority, not commitment. Each item becomes its own phase when prioritized.

### Sound-design power

- [x] **Custom drawn waveforms** — user draws a single-cycle oscillator shape (high-priority post-MVP) — shipped in v0.1.x
- [x] **Wavetable oscillator** — morph between waveforms (4 fixed slots per lane, single morph param 0..1; v3 .beep schema lifts v2 custom_waveform into slot 0)
- [x] **FM oscillator** — two-operator FM with ratio + depth (sine carrier + sine modulator; v4 .beep schema; APVTS supplies defaults for v3 files)
- [x] **Ring modulation** — second oscillator + ring-mod mix (sine carrier × sine modulator with wet/dry crossfade; v5 .beep schema)
- [x] **Additional filters** — formant, comb, bandpass, highpass (5-mode Filter class with type-conditional UI; visualizer plots the active type's analytic response; v6 .beep schema)
- [x] **Additional effects** — chorus, reverb, delay, flanger, phaser (single ModulationEffect slot per lane after distortion; type combo + 3 type-aware sliders; v7 .beep schema)
- [x] **LFOs and a modulation matrix** — 2 LFOs × 4 matrix slots per lane, 11 destinations covering oscillator/filter/effects/gain; block-rate evaluation in B33pProcessor; v8 .beep schema
- [x] **Per-event parameter overrides** beyond pitch (e.g., filter cutoff per hit) — 4 override slots per event covering the modulation matrix's 11 destinations; right-click clip → "Edit overrides..." opens a dialog; v9 .beep schema
- [x] **Event probability, ratcheting, humanize** — three new per-event fields rolled / expanded / jittered at snapshot time; UI controls under the override slots in the same dialog; v10 .beep schema

### Visual feedback

- [x] **Filter response visualizer** — FabFilter/iZotope-style spectrum overlay showing the current cutoff + Q curve so the filter's behaviour reads at a glance; the log-skewed cutoff slider alone feels abrupt without the visual. (Coefficient smoothing was deferred — filter params currently push to the voice only on event trigger, so there's no live-drag zipper to fix yet. Revisit when modulation matrix or per-sample param updates land.)

### Musical timing / DAW integration

Explicitly out of MVP scope — the standalone app is the priority and the "music" framing comes later.

- [x] **Tempo (BPM) + time signature** as pattern attributes — Pattern gains BPM (20..999) + time-sig (numerator 1..32, denominator 2/4/8/16); v11 .beep schema; UI controls in PatternSection
- [x] **Musical snap values** (1/4, 1/8, 1/16, 1/32) derived from BPM — grid combo extended with note-value entries (1/32 .. Whole) that recompute when BPM changes
- [x] **Dual time display** in the pattern ruler (bars/beats + seconds) — ruler now shows seconds along the top half + bar/beat labels along the bottom; playhead readout includes "Bar N.M"
- [x] **DAW transport sync** — when running as VST/AU, follow host tempo, play/stop, and bar position — opt-in "Follow" toggle in PatternSection. Plugin-mode-only by nature (no host = no playhead). Mirrors host play / stop and snaps the playhead to (host_time mod pattern_length); pattern BPM stays independent of host BPM (sound-design users typically want their pattern at a different tempo than the host session). v12 .beep schema persists the toggle.

### Workflow

- [x] **Preset browser** with tags, favorites, search — basic browser ships now (Save Preset / Browse Presets in File menu, ListBox with Load/Delete, per-user Presets directory under app-data); tags, favorites, and full-text search are post-MVP polish on top of this base.
- [x] **Generator presets** — one-click starting points (droid chatter, alarms, weapon charge, UI beeps) — 4 factory presets ship with the app and seed into the user's presets directory on first launch (FM Bell, Resonant Stab, Delay Pad, Ring Mod Robot). Seed is non-destructive — existing files are never overwritten.
- [x] **User-configurable randomization ranges and distributions** — global Randomization Scope slider in PatternSection (0.05..1.0). 1.0 = full-range rolls (legacy behaviour); smaller values constrain rolls to a window centred on each parameter's current value. Single APVTS param so it round-trips through .beep saves automatically.
- [x] **MIDI input** for auditioning — every available MIDI input device routes notes into the selected lane's voice (note 60 = no transposition, higher / lower notes pitch up / down accordingly). Velocity-sensitive. Note-off releases the amp envelope.
- [x] **Polyphony** for voices that benefit from it — 8-voice MIDI keyboard pool added alongside the existing per-lane pattern voices. Pattern playback stays monophonic per lane (overlapping events still re-trigger via the existing amp-env retrigger). MIDI chords now ring together up to 8 notes deep.
- [x] **Batch export** — render N dice-rolled variations in one shot — Variations field in the Export dialog (1..100). Variation 1 captures the user's current patch verbatim; subsequent variations roll all unlocked params before each render. APVTS state is snapshotted before the batch and restored after, so the user's patch survives the dice rolls.

### Distribution

- [x] **VST3 plugin build** — single CMake target via juce_add_plugin produces VST3 alongside Standalone + AU. Loadable in any modern DAW.
- [x] **AU plugin build** (macOS) — same juce_add_plugin target produces an AU component on Apple builds.
- [x] **Additional export formats** — FLAC, OGG, AIFF, 96 kHz — Export dialog gains a Format combo (WAV / AIFF / FLAC / OGG Vorbis) and 88.2 / 96 kHz sample rate options. Filename extension auto-rewrites to match the chosen format. (IMA-ADPCM was originally on this list but dropped — too niche to justify a from-scratch encoder.)
