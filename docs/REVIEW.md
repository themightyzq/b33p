# B33p design review — 2026-05-25

Two-pass critique of the v0.2.0 surface: a designer's tear-down and a cold-eyed first-time walkthrough, merged into a single prioritized list. Findings reference the actual source; where a visual claim requires runtime confirmation, it is marked `[runtime]`.

Scope notes: B33p is a JUCE desktop plugin (Standalone + VST3 + AU). Web-isms (mobile breakpoints, dark-mode toggle, viewport scaling) do not apply. Hierarchy, copy, affordances, empty states, error messages, keyboard, and consistency do.

---

## Critical

### 1. Pattern toolbar over-stuffs and silently clips its own canonical screenshot
**Where:** `Source/UI/PatternSection.cpp:474-507`, `docs/images/hero.png`
The Pattern toolbar lays out Play, Loop, Follow, time readout, Length, Grid, BPM, Time-sig, Scope, Randomize All, Export in a single non-wrapping row via `removeFromLeft/Right`. The committed hero screenshot (canonical per commit `6876f4b`) is missing Follow, BPM, Time-sig, and Scope — because the screenshot's window wasn't wide enough to fit them. The toolbar offers no overflow menu, no wrap, no indication that controls are hidden. The proof that this is broken is sitting in the repo as the README's hero image.
**Why it matters:** A user in a narrow plugin-host window (Reaper split mixer, Ableton sidebar) loses access to BPM, time-sig, DAW Follow, and Randomization Scope with zero visual cue that they exist.

### 2. The canonical hero screenshot is itself out of date
**Where:** `docs/images/hero.png`, `README.md:56-58`
Beyond the missing toolbar controls (item 1), the screenshot also lacks the `(shared across all lanes)` suffix on the Pitch Envelope title, the per-lane background tint, and the level meter in Master. These are all v0.2.0 polish items shipped in the same window. The README leans on this image as the headline visual.
**Why it matters:** First impression — anyone evaluating the project from the README sees a less-finished product than what ships. Several of the most thoughtful polish items are invisible to a passive viewer.

### 3. "Lane" is a top-level menu for a narrowly contextual concept
**Where:** `Source/MainComponent.cpp:329`
Menu bar reads `File · Edit · Lane · Help`. "Lane" sits alongside system-standard menus but only contains operations that act on whatever lane happens to be selected — fundamentally a context menu's job. The menu title even has to be regenerated to read "Copy *Lane 1* voice to all lanes" depending on selection, which gives away that it's not really a top-level concept.
**Why it matters:** Linear/Things/Superhuman never put narrow context controls in the menu bar. A new user reads the menu bar to understand the product's primitives; a `Lane` menu suggests lanes are a peer of files, which they aren't. Belongs inside Edit as a submenu, or as the right-click menu on the lane label.

### 4. `Cmd+/` for About is non-standard and self-defeating
**Where:** `Source/MainComponent.cpp:257-262, 408`
About is the only Help menu item, bound to `Cmd+/`. Standard macOS practice for About is no shortcut (it lives in the app menu) or `Cmd+,` for Preferences. `Cmd+/` is the comment-toggle in every code editor and unrelated elsewhere. The shortcut is documented inside the About dialog — meaning a user can only learn the shortcut by first invoking the menu item it shortcuts.
**Why it matters:** Surprises power users and teaches first-timers nothing.

### 5. Help menu has no "Keyboard Shortcuts" entry; the full reference is buried inside the About dialog
**Where:** `Source/MainComponent.cpp:407-409`, About body at `:585-620`
The Help menu contains exactly one item: About. The About dialog body embeds the full keyboard shortcut reference (Space, Shift+Space, Cmd+A/C/V, Esc, arrows). A user looking for "what does spacebar do?" must open About — a place they expect version info and credits.
**Why it matters:** Discoverability for the single most common new-user question.

### 6. Destructive lane operations execute without warning
**Where:** `Source/MainComponent.cpp:448-465` — `LaneClear`, `LaneResetVoice`, `LaneCopyToAll`, `LaneDiceAll`, `LaneGenerate`
Five of the six Lane menu items mutate state irreversibly-feeling (undo exists, but the path isn't telegraphed). `Reset Lane 1 voice to defaults` and `Clear All Events in Lane 1` are one-click away with no confirmation step, no diff preview, and no "wipe this lane's existing pattern" warning before `Generate Random Pattern in Lane 1` overwrites it. File ▸ New and File ▸ Open both go through `confirmDiscardThen()`; the Lane menu does not.
**Why it matters:** The user's expectation is "destructive actions ask first." Undo softens the blow but doesn't fix the design.

### 7. Mute / Solo lane buttons differentiate state by color alone
**Where:** `Source/UI/PatternGrid.cpp:68-97, 134-140`
Per-lane `M` and `S` are 22 px buttons with 11 pt single-letter labels. Active states use red (190, 60, 60) for Mute and yellow (220, 200, 60) for Solo on grey (34, 34, 34). Inactive vs active is distinguished by tint only — no fill change, no icon swap, no border. Red and yellow are the worst pair for the most common form of color blindness (deuteranopia / protanopia).
**Why it matters:** The fastest interactive lane controls on the screen rely on color the user may not see. Add a fill-vs-outline distinction and a clear icon.

### 8. Empty pattern hint is a single run-on sentence cramped on one line
**Where:** `Source/UI/PatternGrid.cpp:693-695`
The hint reads "Drag in a lane to draw a beep, or double-click for default size. Drag a beep to move it (vertically to switch lanes), drag its edges to resize." — 137 characters laid out as centered text inside the empty pattern grid. The hero screenshot shows it visibly truncating mid-instruction.
**Why it matters:** It's the most important onboarding text in the product — the single sentence that teaches the entire pattern editor — and it reads like a stage direction. Two short lines or three bullets would land. Better still: pair with a faint ghost-event showing what a clip looks like.

### 9. Pitch Envelope empty state is a single dot and a 60-character hint
**Where:** `Source/UI/PitchEnvelopeEditor.cpp:89-98`
At first launch the Pitch Envelope is a wide dark canvas containing a single orange dot in the lower-right corner and centered text "Click to add pitch points. Drag to shape, right-click to delete." There is no cursor change on hover, no grid, no center-line, no axis labels, no preview of what a curve will look like. A right-click affordance for delete is mentioned but not signalled visually.
**Why it matters:** Drawable pitch envelopes are one of B33p's signature features; the empty state actively hides that. A faint default flat curve at zero semitones + a hover cursor change would do most of the work.

### 10. Every voice section title carries "(Lane N)" — five repetitions of the same context
**Where:** `Source/UI/Section.cpp:40` and every section that calls `setTitleSuffix`
Oscillator (Lane 1), Amp Envelope (Lane 1), Filter (Lane 1), Effects (Lane 1), Master (Lane 1), Modulation Effects (Lane 1), Modulation (Lane 1) — seven section titles, all suffixed identically. The accent tint already encodes the lane. The lane indicator should live at the editor level once (badge, dropdown, or lane-header strip), not be repeated as suffix copy.
**Why it matters:** Linear/Vercel-quality UI uses repetition for rhythm, not noise. The (Lane N) suffix is the loudest, most-repeated piece of typography on screen, and it tells the same story every time.

---

## High Impact

### 11. Two TODO items marked done don't match what ships
**Where:** `TODO.md:213` (Audio settings menu) and `TODO.md:221` (custom-waveform footer text)
TODO says the Audio settings menu lives in a custom "Options menu"; in reality it's exposed via JUCE's standalone-wrapper gear icon (deliberate choice — see `MainComponent.h:27`). TODO proposes the custom-waveform footer text "Drawing replaces the default sine cycle"; shipped text is more useful. Code is fine in both cases; the planning doc has drifted.
**Why it matters:** CLAUDE.md mandates TODO is the source of truth. Wording drift erodes that.

### 12. The Audio settings entry point exists but is not discoverable as such
**Where:** Implicit in `MainComponent.h:27`; only surfaces via JUCE's standalone wrapper hamburger icon
A user whose default audio device isn't usable on launch sees silence and has no menu bar entry pointing them to the fix. The fix is one click — but the click is in JUCE-rendered chrome that doesn't match the rest of the app, and there's no in-app pointer to it.
**Why it matters:** The whole motivation for the v0.2 polish-sweep item was "user has no in-app recovery if default output isn't usable." Recovery is there; recoverability is not.

### 13. Export defaults to 48 kHz, not 44.1 kHz
**Where:** `Source/UI/ExportDialog.cpp:15-16, 100` — `sampleRateCombo.setSelectedId(idForIndex(5), …)`
The Export dialog opens with 48 kHz pre-selected. Defensible for video-leaning workflows; surprising for the README's explicit positioning ("Star Wars droids, retro games, sci-fi UIs, chirpy gadgets" — all music/web-distribution contexts where 44.1 kHz is the de facto). The 8 kHz / 11.025 kHz / 16 kHz lo-fi options ahead of it in the combo also suggest the user is being nudged downward, then handed 48 kHz anyway.
**Why it matters:** Most users won't audit the dropdown. Either default to 44.1 or remember the user's last choice.

### 14. Export filename's extension doesn't update when the user changes the format combo
**Where:** `Source/UI/ExportDialog.cpp:88-92, 248`
Format → extension rewrite happens at *submit*, not on combo change. While the dialog is open, a user who picks "FLAC" still sees `b33p_export.wav` in the file field. Functionally fine (submit will fix it), but it makes the dialog feel desynchronized — a small thing that adds up.
**Why it matters:** Either rewrite live on `onChange`, or hide the extension from the displayed filename entirely.

### 15. 100-variation batch export has no time estimate and no confirm
**Where:** `Source/UI/ExportDialog.cpp:122-128`
Variation slider goes 1..100. Selecting 100 + Export immediately begins a render that could be 30-90 seconds depending on patch length, with the UI locked. No "this will render 100 files, ~45 s estimated" pre-flight, no cancel, no per-file progress beyond the existing single-render indicator.
**Why it matters:** A drag-the-slider-to-the-end-and-click curiosity move turns into "did the app freeze?"

### 16. ExportTask blocks the UI thread on long batches
**Where:** `Source/UI/ExportTask.{h,cpp}` — verify against the runtime
The progress indicator suggests intent to keep things responsive; in practice, the snapshot-roll-restore loop runs synchronously per variation. `[runtime]` check needed for whether the UI stays interactive (Esc cancel, drag the dialog, etc.).
**Why it matters:** Responsiveness during long batches.

### 17. AmpEnvelopeVisualizer updates only on `parameterChanged`, not during slider drag
**Where:** `Source/UI/AmpEnvelopeVisualizer.cpp:60-65`
JUCE's APVTS `parameterChanged` callback fires on the audio thread after the parameter commits. The visualizer redraws then. During an active drag, the curve may stay static until release. `[runtime]` confirmation needed for the specific JUCE 8 behavior.
**Why it matters:** The point of a live ADSR visualizer is the live part. If it lags the drag, it becomes a result preview, not a control hint.

### 18. LabeledSlider's dice/lock buttons fight the parameter label for attention
**Where:** `Source/UI/LabeledSlider.cpp:9-13, 69-72`
The dice+lock pair sits in a 20-px-tall row beneath every knob, occupying ~half the knob's width each. The parameter name (11 pt) sits directly above them, the value readout (11 pt) above that. At typical knob sizes (100-150 px wide), the dice/lock buttons read with as much weight as the parameter label.
**Why it matters:** Hierarchy collapse. A user's eye should land on parameter name → value → control → randomize/lock as a clear descending order. Today it's roughly all equal.

### 19. Filter / Oscillator section reflows are abrupt
**Where:** `Source/UI/OscillatorSection.cpp:70-79`, `Source/UI/FilterSection.cpp:39-51`
Switching waveform (Sine → FM, Sine → Wavetable, etc.) or filter type (Lowpass → Formant) shows/hides 2-3 sliders and re-flows the remaining knobs to fill the new width. No fade, no transition; the layout just snaps. `resized()` also resizes the remaining knobs' aspect ratios, so an unchanged "Cutoff" knob suddenly looks bigger or smaller depending on its new neighbor count.
**Why it matters:** Confidence. The control that didn't change shouldn't visibly resize.

### 20. ModulationSection has no empty-state guidance
**Where:** `Source/UI/ModulationSection.{h,cpp}`
A new user opening the Modulation section sees: "Slot 1 [Source dropdown · Destination dropdown · Amount slider]" four times, plus two LFO rate/shape controls. Every dropdown defaults to "None". No copy explaining what a slot does, what an LFO does, or that nothing happens until both Source and Destination are non-None.
**Why it matters:** The most powerful section in the editor has the least onboarding. "Choose a Source and Destination to start modulating" as an empty-state row would close the gap.

### 21. ModEffectsSection labels change to non-applicable values when type is None
**Where:** `Source/UI/ModEffectsSection.cpp:16-23, 54-71`
When the user sets Modulation Effect type to None (bypass), the three sliders are disabled (correct), but the labels still read the last active type's labels ("Rate", "Depth", "Mix"). The labels should either stay neutral ("Param 1 / 2 / 3") or fade with the sliders, not advertise functionality that isn't active.
**Why it matters:** Confusing affordance — looks broken or unresponsive.

### 22. Snap-target preview line is too faint to track
**Where:** `Source/UI/PatternGrid.cpp:752-757`
During drag, the valid-snap guide draws at alpha 0.55 over the dark background. The clamp-wall variant is 0.85. On a bright screen the 0.55 alpha is borderline invisible — releasing a clip into "approximately the right place" relies on a line the user can barely see.
**Why it matters:** Snap is a precision interaction. Faint feedback erodes the precision.

### 23. Low-velocity clips collapse to an 8-pixel floor
**Where:** `Source/UI/PatternGrid.cpp:498-507`
Clip height encodes velocity, with a hardcoded `minH = 8 px` floor. Any velocity ≤ ~16% renders at the floor, indistinguishable from any other low-velocity clip. The top-edge drag affordance for setting velocity becomes invisible/imprecise in that range.
**Why it matters:** Velocity is per-event design; if the visual encoding flattens at the bottom of the range, users will assume their edits stopped working.

### 24. Lane labels look static — no cursor change or visual cue that double-click renames
**Where:** `Source/UI/PatternGrid.cpp:36-43`
`nameLabel.setEditable(false, true, false)` quietly enables double-click-to-edit. A tooltip exists. There is no cursor change on hover, no border state, no faint pencil/edit affordance.
**Why it matters:** Buried feature. The only way to discover lane rename is to read the docs (or the tooltip, which only some users hover for).

### 25. Master gain readout has no unit
**Where:** `Source/UI/MasterSection.cpp:58`
The Master Gain knob displays "1.00" — no `dB`, no `×`, no `%`. The parameter is 0..10 linear (linear gain, not dB). A user can't tell from the readout whether `2.00` means +6 dB, 200%, or 2× linear (it's the third, but no one will guess that).
**Why it matters:** Audio users speak dB. Either show dB (with linear under the hood) or label `× linear`.

### 26. Save-Preset failure message doesn't tell the user what to do
**Where:** `Source/MainComponent.cpp:536-544`
On preset save failure the alert says "The name may be invalid or the preset directory is unwriteable." Both halves are non-actionable: the user can rename, but the dialog doesn't say which characters are invalid; they can fix permissions, but the dialog doesn't say where the directory is.
**Why it matters:** "Tell me what to do" is the rule for error copy. This message tells the user what *might* be true.

### 27. Preset browser has no factory presets surfaced in the list, no empty state
**Where:** `Source/UI/PresetBrowserDialog.cpp:48-83`, `Source/State/PresetManager.cpp:37-55`
On first launch the factory presets are seeded into the user preset directory (good). But the browser shows them as an undifferentiated list with no badge — they look identical to user-saved presets. A user who Delete-presses through them clears the factory set with no warning. Also, if the directory is empty for any reason, the list is just blank — no "Save your first preset" pointer.
**Why it matters:** Factory presets are a discoverability tool; once they look like user files, they lose that role.

### 28. Window title is identical between Standalone and plugin modes
**Where:** `Source/MainComponent.cpp:324`
Reads "b33p — Untitled" in both contexts. A plugin user opening their DAW project has no way to know whether the title's filename refers to a `.beep` they once loaded, a preset they're editing, or nothing at all. (Plugin state typically lives inside the DAW session — there is no `.beep`.)
**Why it matters:** Cognitive load when context-switching between standalone and plugin instances.

### 29. Open Recent submenu greyed-out when empty looks like a bug
**Where:** `Source/MainComponent.cpp:371`
Empty MRU disables the submenu. A greyed-out menu item with no children reads to most users as "feature broken" rather than "feature inactive." Better: enabled submenu containing a single disabled `No recent files` item.
**Why it matters:** First-launch state shouldn't look broken.

---

## Nice to Have

### 30. Section accent strip is 2px tall at 0.85 alpha — almost invisible
**Where:** `Source/UI/Section.cpp:44-53`
Per-lane accent tint lives on a 2-pixel strip under each section title at 0.85 alpha against a dark background. The intent (visual lane identity) is correct; the execution is too quiet to do the job. 4 px at full alpha would carry without becoming heavy.

### 31. WaveformEditor footer is informative but verbose
**Where:** `Source/UI/WaveformEditor.cpp:153-159`
"Click + drag to draw. Custom mode plays Slot 1; Wavetable mode blends all four via the Morph slider." Reads as two thoughts crammed together. Splitting into a "Drawing on Slot 1" headline near the canvas and "Wavetable mode blends all four slots" beside the Morph slider would tell the same story without the footer being a paragraph.

### 32. Slot numbering is 1-based in UI, 0-based internally
**Where:** `Source/UI/WaveformEditor.cpp:153-159` and `Source/Pattern/Pattern.h` overrides
Friendly choice on the UI side. Surfaces as a footnote because anyone debugging via the file format or test fixtures sees the 0-based index. Cosmetic.

### 33. Bar labels in the ruler can collide
**Where:** `Source/UI/PatternGrid.cpp:658-671`
46-px-wide bar labels in `centredLeft` justification with no collision avoidance. At 140 BPM in 4/4 inside a 5 s pattern, bars are ~1.7 s apart; in a narrow plugin host they will visibly overlap.
Add label-skip logic that hides every other bar label as density rises.

### 34. Ruler tick hierarchy is too narrow in contrast
**Where:** `Source/UI/PatternGrid.cpp:614-655`
Grid lines (40, 40, 40), 1-s ticks (100, 100, 100), beat ticks (~150, 150, 150), bar ticks (~190, 190, 190). The dynamic range is small enough that the user's eye can't pre-attentively separate "grid" from "ruler" from "beat".

### 35. Multi-select state has no operation hint
**Where:** `Source/UI/InspectorStrip.cpp:164-166`
Selecting N events shows "N events selected" with all numeric fields hidden — correct. But there's no line telling the user "Delete · ↑/↓ · Cmd+C / Cmd+V apply to the whole selection." A single line of dim help text would close the loop.

### 36. Hovered lane has no tint
**Where:** `Source/UI/PatternGrid.cpp:574-578`
Selected lanes get a 0.10-alpha tint; unselected lanes 0.04. No hover state at all. A drag from the wrong starting point lands in the wrong lane with no pre-attentive warning.

### 37. Playhead at non-zero "parked" position is dim
**Where:** `Source/UI/PatternGrid.cpp:765-775`
The playhead is a 1.5-px line at (140, 140, 140), which is dimmer than the brighter bar ticks. When stopped, the playhead acts as the paste-anchor for Cmd+V — it should be the most prominent vertical line in the pattern, not blend in with the ruler.

### 38. The empty inspector copy is laconic
**Where:** `Source/UI/InspectorStrip.cpp:` empty-state path
"Click an event to edit it." is correct but doesn't sell the inspector's purpose. "Select a clip to edit pitch, velocity, and overrides" tells a user what they can do here.

### 39. About dialog body hardcodes the keyboard shortcut table
**Where:** `Source/MainComponent.cpp:585-620`
The shortcut list is a string literal inside the About dialog. Any added shortcut needs hand-syncing. A small static table consumed by both the About body and (eventually) a dedicated "Keyboard Shortcuts" Help item would avoid drift.

### 40. Unsaved-changes prompt uses indirect phrasing
**Where:** `Source/MainComponent.cpp:195`
"This project has unsaved changes. Save before closing?" reads slightly weaker than "Save changes to `untitled.beep` before closing?" — the latter pulls the filename into the question, which is what a user mentally tracks.

---

## What I could not verify without runtime

- Whether AmpEnvelopeVisualizer / FilterResponseVisualizer redraw live during slider drag, or only after release (item 17 / spot-check for Filter).
- Whether the ExportTask UI stays responsive during a 100-variation batch (item 16).
- Whether the JUCE standalone wrapper's audio-settings gear icon is genuinely discoverable in the v0.2.0 bundle (item 12).
- Color contrast measurements against the actual rendered theme (items 7, 30, 34, 37) — code values cited; perceptual verification needs a screenshot.
- Whether the snap-preview guide is, in fact, hard to see at 0.55 alpha on the canonical theme (item 22).

Each of these is worth ten minutes during smoke-testing the v0.2.0 archives.

---

## Cross-cutting observation: the gap between built and felt

B33p has shipped enormous functional surface — a full modulation matrix, polyphony, MIDI, host transport sync, four export formats, twelve versioned schema migrations. The audit found zero correctness issues and zero CLAUDE.md violations. What it didn't get yet is the second pass that takes a fully-built product and makes it feel obviously well-made: the moments when the toolbar should know it's too narrow, when the empty pattern should teach the gesture by showing a ghost, when the user's first instinct ("what does space do?") should land somewhere richer than the About dialog.

The fixes in Critical 1-10 are mostly small in code; their impact is disproportionate because each one removes a daily friction the user doesn't articulate but does feel.

---

# Plugin designer pass — 2026-05-25

Second review of B33p — same date, different lens. The first pass treated B33p as a desktop app: window layout, copy, menus, empty states. This pass treats it as a plugin a working producer is about to drop on a track mid-session and judge against the craft bar set by u-he, Baby Audio, Soundtoys, Goodhertz, Arturia. B33p's lane is **characterful / experimental** — sound-design playground, not surgical tool — so the bar is "playful, immediate, opinionated," not "clinical precision."

Findings are independently actionable. Some items here partially overlap or refine items from the first pass; cross-references are noted.

**Corrections to first-pass claims** (verified during this pass and worth recording before the new findings):

- *The editor IS resizable.* `B33pEditor.cpp:19` calls `setResizable(true, true)`. The wrap-layout shipped in commit `33f8591` therefore benefits standalone users too — not just plugin hosts. The first pass's "Editor stays at fixed 1500 pt default" framing was wrong.
- *Randomizer locks are persisted.* `ProjectState.cpp:163-170, 539-540` — locks save/load correctly across .beep round-trips and DAW state recall.
- *Slider text boxes are editable.* `LabeledSlider.cpp:20` — `setTextBoxStyle(..., isReadOnly=false, ...)`. The issue is discoverability (double-click the text box vs right-click), not absence — moved from Critical to High Impact below.
- *"Randomize All" is properly wrapped in one undo transaction.* `ParameterRandomizer.cpp:125-128`. The first pass implied otherwise in some phrasings — confirmed it's correct.

---

## Critical

### P1. No host-bypass support
**Where:** `Source/State/B33pProcessor.cpp:processBlock` — no `isBypassed()` check.
The host's bypass button or automation lane doesn't reach the audio. When a user hits Bypass on the track, B33p keeps generating audio normally. This is a host-contract violation; every shipping plugin honors host bypass. Add an `if (isBypassedByHost()) { buffer.clear(); midi.clear(); return; }` early-exit, ideally with a click-free crossfade.

### P2. No latency reporting to host
**Where:** `B33pProcessor::prepareToPlay` / `processBlock` — `setLatencySamples(...)` never called.
Currently the plugin has no introduced latency, so reporting 0 is technically correct — but the plumbing isn't wired at all, so any future addition (oversampling, look-ahead limiter, FFT) will silently throw automation alignment out of sync in DAWs that rely on PDC. The habit of *always* calling `setLatencySamples(0)` explicitly in `prepareToPlay` documents the contract and makes future additions safe.

### P3. No tail-length reporting
**Where:** `B33pProcessor.h:221` — `getTailLengthSeconds() const override { return 0.0; }`.
B33p's modulation effect slot includes Reverb (`juce::Reverb`) and Delay (`juce::dsp::DelayLine` with feedback). When the user stops transport, the host cuts the buffer at the stop boundary because the plugin claims zero tail — so the reverb wash and delay echoes vanish mid-decay. Compute a tail from the active mod-effect state (reverb size → ~2-3 s; delay time × max sensible feedback iterations) and return it.

### P4. No A/B compare
**Where:** Doesn't exist anywhere — no UI control, no state-snapshot logic in `B33pProcessor`.
Less load-bearing for a sound-design synth than for a mix processor, but still expected at this build level. A user designing a sound iterates: tweak → "is that actually better?" → without A/B they have to commit, audition, undo, audition again. Soundtoys/Sonnox standard is two slots (A and B) with a swap button and a "copy A→B" button. Two snapshots of the APVTS state + a button bar; the existing `ProjectState::toXmlString()` already does the serialization work.

### P5. No in-plugin Undo button
**Where:** `MainComponent.cpp:256-261` — Cmd+Z routes to `processor.getUndoManager().undo()`, which works in standalone but is captured by the *host* in VST3/AU mode. There's an Edit menu Undo (line 387-390) but menu navigation breaks flow.
FabFilter, iZotope, Soundtoys all ship a small Undo/Redo button on the plugin surface specifically because hosts capture Cmd+Z. Two icon buttons in the top bar (left of the lane indicator) costs almost nothing visually and rescues the most-used keyboard shortcut.

### P6. Loading a preset destroys undo history
**Where:** `Source/UI/PresetBrowserDialog.cpp:requestLoadSelected` → `MainComponent::confirmDiscardThen` → `presetManager.loadPreset` → `ProjectState::readFromFile` → APVTS `setStateInformation`. None of the steps in this chain begin an undo transaction.
User browses presets, picks one, doesn't like it, hits Cmd+Z — nothing happens, the previous patch is gone. Wrap `ProjectState::load` in a single undoable action (snapshot pre-load APVTS state, restore on undo). This is the same pattern `UndoableActions.h` already uses for pattern and pitch-curve edits.

### P7. Selected lane not persisted in plugin state
**Where:** `B33pProcessor::selectedLane` is an `std::atomic<int>`; `ProjectState.cpp` has zero references to it (verified with `grep`).
DAW save → close → reopen → user is back on Lane 1 regardless of where they were. Mid-session sound design loses context constantly. Persist as a `selected_lane` property on the root ValueTree.

---

## High Impact

### P8. Plugin is shipping on stock JUCE LookAndFeel_V4
**Where:** Repo-wide `grep` for `LookAndFeel` / `setLookAndFeel` returns nothing. The default chrome is showing: rotary sliders with the "cricket-leg" line indicator (`LabeledSlider.cpp:19` — `RotaryHorizontalVerticalDrag` with no custom paint), default ComboBox gradient + arrow (every section's type/waveform combo), default TextButton round-rect on most buttons, default AlertWindow on confirms.
For a clinical EQ this would be defensible. For a synth in the same lane as Baby Audio Tuna Knob, u-he Bazille, or Goodhertz Lossy — where the *interface itself* is part of the sound-design vibe — this telegraphs "JUCE tutorial project" the moment a producer drops it on a track. A custom `LookAndFeel` subclass overriding `drawRotarySlider`, `drawComboBox`, `drawButtonBackground`, and `drawPopupMenu*` is the minimum entry fee for this lane. Pick one accent color and one knob silhouette; ship.

### P9. No fine-adjust modifier on knob drag
**Where:** `LabeledSlider.cpp:19` — uses JUCE's default rotary drag with no modifier-key sensitivity overrides.
Shift-drag-to-fine-tune has been a synth standard since Massive in 2007. Currently a user trying to dial in 1.5 ms attack vs. 1.2 ms either gets it on the first drag or doesn't. Add `slider.setVelocityModeParameters(...)` or override `mouseDrag` to detect `ModifierKeys::shiftModifier` and scale the drag delta down 10×.

### P10. No right-click context menu on knobs
**Where:** Default JUCE Slider behavior + no override. The pattern grid has right-click context (clip → Delete / Duplicate / Edit overrides; empty grid → Generate / Clear) but the knobs have nothing.
Reach-for-the-mouse moments that should be one click: "set to default" (currently double-click works but isn't signaled), "enter value..." (currently double-click on the text box works but isn't signaled), "copy parameter value", "paste parameter value", "MIDI learn", "host automate". JUCE's `ModalPopupMenu` is one method; the LookAndFeel work in P8 absorbs the styling.

### P11. Editor size not persisted in plugin state
**Where:** `B33pProcessor::getStateInformation` (per code structure) writes the `ProjectState` XML; no editor-bounds storage. The editor *is* resizable (P-correction above), so users will resize — and lose it on the next DAW reopen.
Save `editor_width` and `editor_height` properties on the root ValueTree at editor destruction; restore in the editor constructor. Or use the host-provided editor-bounds mechanism if the JUCE wrapper exposes it cleanly.

### P12. No min/max resize constraints
**Where:** `B33pEditor.cpp:19` — `setResizable(true, true)` is called, but no `setResizeLimits` or `ComponentBoundsConstrainer` is installed.
A user can shrink the window to where every section header is unreadable, or balloon it past 4K. The wrap-toolbar path handles 1200-1440 pt gracefully; below ~900 the voice sections themselves break. Set a sensible minimum (probably 1200×900) and a generous maximum (3000×2000).

### P13. Filter response visualizer doesn't animate during slider drag
**Where:** `Source/UI/FilterResponseVisualizer.cpp` — repaints on `parameterChanged` callback only. In some hosts that callback fires on slider *release*, not during drag — making the visualizer a "result preview," not a control hint. (First-pass item #17 flagged this for `AmpEnvelopeVisualizer`; same root cause and same impact applies here.)
The fix is the same: attach a Slider::Listener that calls `repaint()` on `valueChanged`, regardless of whether the host already fired the parameter callback.

### P14. Modulation matrix shows no active modulation
**Where:** `Source/UI/ModulationSection.cpp` — slot rows are static `(source combo)(destination combo)(amount slider)`. Nothing animates when a slot is live.
The whole point of a mod matrix is "see this knob move because of that LFO." Currently you set up a routing and… stare at static sliders. Two cheap wins: (a) when a slot's destination resolves to a real parameter, glow that parameter's knob ring during pattern playback at the modulated value, or (b) draw a small moving indicator on the matrix-amount slider showing where the modulated value currently sits. Either gets the "the patch is alive" feeling that u-he Bazille / Massive X earn.

### P15. Stock JUCE ComboBox chrome on every dropdown
**Where:** Every `OscillatorSection`, `FilterSection`, `ModEffectsSection`, `ModulationSection`, `PatternSection`, `ExportDialog`, `PresetBrowserDialog` uses `juce::ComboBox` with no styling.
The dropdown chrome doesn't match the rest of the (dark, minimal) palette. The arrow looks legacy. Fix bundles with P8 — once a `LookAndFeel` is in place, override `drawComboBox` once and everything inherits.

### P16. No CPU display
**Where:** Nowhere. The plugin runs ~22 modulation parameters per lane × 4 lanes + mod-fx chain, but the user has no visibility into actual cost. u-he plugins put a tiny `CPU 4%` readout in the corner so a user nailing complex patches knows when they're about to redline.
Lowest-effort version: a `juce::AudioProcessor::getProcessingTimeStats()`-derived percentage, painted in the bottom-right of the master section.

### P17. Distortion has no oversampling option
**Where:** `Source/DSP/Distortion.cpp:28` — `tanh()` waveshaper, no oversampling.
At high drive on transient material, this aliases audibly. Bitcrush is intentionally aliased (lo-fi is the point — fine), but Distortion is a *creative* effect that benefits from a clean version. A 2×/4× oversampling toggle on the master gives users the choice. `juce::dsp::Oversampling` is a one-class drop-in.

### P18. Master level meter has no clip indicator and no headroom marks
**Where:** `Source/UI/MasterSection.cpp:113-128` — three-zone green/amber/red bar but no separate clip badge, no headroom reference lines.
A user can't tell whether the red zone means "+0.5 dBFS, about to clip" or "+6 dBFS, clipping hard." Add: a separate `CLIP` indicator (small red square that latches on for ~2 s after any sample > 0 dBFS), tick marks at -12 / -6 / -3 / 0 dBFS, and peak-hold ticks that decay over ~1.5 s.

### P19. No preset metadata (author / description / tags / category / BPM)
**Where:** `PresetManager.{h,cpp}` saves only the filename + raw `.beep` state.
Browser is a flat alphabetical list. At 50+ user presets it becomes unmaintainable. Add a minimal metadata layer: tags array, author string, one-line description. Surface in the browser as a right-hand info pane. Adds one ValueTree node (`<Meta>...</Meta>`) to the preset format — back-compat is trivial.

### P20. No audition-while-browsing in the preset browser
**Where:** `PresetBrowserDialog.cpp:requestLoadSelected` — load only on explicit button click.
u-he, Arturia, NI Komplete Kontrol: select a preset in the list → it auto-loads with a brief audition trigger. Currently a user can't preview without committing — every preset audition is a Cmd+Z risk (and per P6, the undo doesn't even work).
Easiest path: on list-selection change, load the preset temporarily and trigger an audition. On dialog cancel, restore the snapshot.

### P21. No init / default preset
**Where:** No UI button. Lane menu has "Reset Lane voice to defaults" but only per-lane.
A "global init" — restore every lane to APVTS defaults + clear pattern — is missing. A user wanting to start over has to: New project → confirm discard → new instance opens. Three clicks for what should be one.

### P22. Factory presets not visually distinguished
**Where:** `PresetBrowserDialog.cpp:80` — list shows filename only.
Factory and user presets are indistinguishable. Once a user accidentally overwrites "FM Bell" with their own patch, the factory version is gone (and `seedFactoryPresetsIfMissing` won't restore it because the file already exists — per `GeneratorPresets.h:56-59`).
Tag factory presets with a different color/icon in the list, and add a "Restore Factory Presets" option in the Lane or File menu.

### P23. No next/previous preset arrows in the main UI
**Where:** No control on the main editor surface — only via the modal browser dialog.
The Massive X / Diva pattern: small `‹ Preset Name ›` widget at the top, click arrows to step through the user/factory list. Costs ~80 pt of horizontal space and saves four clicks per preset audition.

### P24. No copy-voice between plugin instances
**Where:** No menu action, no clipboard surface for voice patches.
A producer who has Lane 1 of an instance dialed in can't quickly grab that voice into a different instance. The pattern grid has Cmd+C/V for events; voice patches don't. Add `Lane ▸ Copy voice to clipboard` / `Lane ▸ Paste voice from clipboard` — serialize the single-lane subtree of `ProjectState` to the system clipboard.

### P25. Slider value entry exists but is undiscoverable
**Where:** `LabeledSlider.cpp:20` — text box is editable (`false` = not-read-only), but there's no visible affordance.
Double-clicking the text box does enable typing — but no one will guess this without docs. The standard signal is either: (a) right-click → "Enter value..." in the context menu (also doesn't exist — see P10), or (b) hover the text box → cursor changes to text I-beam, faint underline appears. Cheap: change the text box cursor on hover.

### P26. Save Preset has no overwrite confirmation
**Where:** `MainComponent.cpp:539` calls `presetManager.savePreset(name)` without checking name collisions. `PresetManager.cpp:76-78` comment says "caller is expected to confirm" — the caller doesn't.
User saves a preset, types a name that matches an existing factory or user preset, hits OK — silent overwrite, no warning. Standard convention is an `AlertWindow` asking "A preset named 'FM Bell' already exists. Overwrite?".

### P27. VST3 / AU category not explicitly declared
**Where:** `CMakeLists.txt:62-77` — `juce_add_plugin` block has no `VST3_CATEGORY` or `AU_MAIN_TYPE`.
Defaults to "Instrument | Synth" via `IS_SYNTH TRUE`, which works, but explicit `VST3_CATEGORY "Instrument|Synth|Sampler"` improves discoverability in host plugin pickers that filter by subcategory. AU's `AU_MAIN_TYPE` defaults to `kAudioUnitType_MusicDevice` correctly — explicit declaration documents intent.

---

## Nice to Have

### P28. Numeric readouts aren't tabular
**Where:** Most labels and slider readouts use `juce::FontOptions(11.0f)` or the JUCE default proportional font. Numerals are proportional-spaced, so during knob drag a value like "1.234" → "1.245" can visibly shift left/right because `2` and `3` aren't the same width.
Use a tabular-figures variant (most system fonts have it, accessible via `juce::Font::withExtraKerning` or by switching to a monospaced font for readouts). Or just hand-pick a font like JetBrains Mono or IBM Plex Mono for numeric cells.

### P29. Button styling inconsistency
**Where:** Play button has custom green (`PatternSection.cpp:142` ~`(60,140,70)`); Audition has an amber flash (`MasterSection.cpp:72-73`); Mute/Solo have per-lane colors; most other buttons (Loop, Follow, Export, Randomize, file dialog buttons) use stock TextButton chrome.
The custom-colored buttons stand out, but the inconsistency reads as "some buttons got attention, most didn't" rather than a deliberate hierarchy. After the LookAndFeel work in P8, decide on a hierarchy: primary action (green/accent), secondary action (subtle), and let mute/solo/follow remain their own state-toggle visual. Then apply consistently.

### P30. Modulation amount uses LinearHorizontal instead of rotary
**Where:** `Source/UI/ModulationSection.cpp:259-261` — `slider.setSliderStyle(juce::Slider::LinearHorizontal)` for amount; rest of the plugin is rotary.
Breaks the synth's visual rhythm. Switch to bipolar rotary (centered at zero with the indicator sweeping ±) to match the rest of the editor and signal "this is a bipolar mod amount" with one glance.

### P31. AmpEnvelope visualizer has no playhead
**Where:** `Source/UI/AmpEnvelopeVisualizer.cpp` — static ADSR curve.
Painting a vertical line moving through the curve during note playback (or pattern event playback on the selected lane) makes the visualizer *active* rather than decorative — see P14's general "show what's happening to the audio" principle. Combine with velocity-sensitive height for extra signal.

### P32. PitchEnvelope canvas has no axis labels
**Where:** `Source/UI/PitchEnvelopeEditor.cpp` — no time tick marks (0s, 0.5s, 1s) along the X-axis, no semitone marks (+12, 0, −12) along the Y-axis.
Currently the user is drawing a curve in unitless space. Adding subtle tick marks with labels (8 pt grey, near the axes only) gives the curve concrete meaning. Pair with the playhead from P31.

### P33. Per-event overrides have no indicator in the grid
**Where:** `Source/UI/PatternGrid.cpp:498-535` — clip rendering checks selection state but not whether the event has any non-default overrides set.
A user setting up "every 3rd event drops filter cutoff" can't see at a glance which events are special. A small ⏵ or `▪` glyph in the clip corner (only drawn when `event.overrides.any() || event.probability < 1 || event.ratchets > 1 || event.humanizeAmount > 0`) makes the per-event work visible.

### P34. Mute / solo state not echoed in voice editor sections
**Where:** Lane label in the grid shows the M/S toggle state visually; the voice editor sections above (Oscillator (Lane 2), Filter (Lane 2), etc.) show no indication when Lane 2 is muted or soloed.
A faint overlay or a `MUTED` chip in the section header tells the user "you're editing a voice that's currently silent."

### P35. Randomize is silent and instant
**Where:** `MasterSection.cpp:Randomize Lane button` triggers ParameterRandomizer → APVTS attachments → knobs pop to new values.
No transition, no flourish, no toast confirming the action. A 60 ms tween from old to new value on every affected knob (driven by `juce::ValueAnimator` or the simpler `Component::animateComponent` style) sells "the dice rolled" in a way an instant pop doesn't.

### P36. No mousewheel scroll on knobs
**Where:** `LabeledSlider.cpp` — no `mouseWheelMove` override; JUCE Slider's default scroll wheel behavior on rotary depends on the host platform and is often inert.
Vertical mouse-wheel = +/- one step is standard ergonomic baseline. Trackpad two-finger scroll inherits this on macOS for free if the override is sane.

### P37. No keyboard navigation in the preset browser
**Where:** `PresetBrowserDialog.cpp` — the list responds to clicks, no Up/Down/Enter handling.
A producer doing rapid preset auditioning should be able to focus the list, tap Down arrow to walk the list with auto-load (per P20), and hit Enter or Esc to commit/dismiss. `juce::ListBox` supports this with the right keyboard listener.

### P38. Bus configuration is stereo-only with no I/O flexibility
**Where:** `B33pProcessor::BusesProperties()` declares stereo output only.
B33p is a mono-character instrument (pitched beeps); a `mono` bus might suit some hosts (Logic surround, Ableton mono channels) better. Adding `withOutput("Mono", AudioChannelSet::mono(), false)` lets hosts pick. Low priority — most producers will accept stereo-duplicated.

---

## Cross-cutting observation: characterful synths sell the *feeling* of the patch

The first pass found B33p had built almost everything functional and just needed the second-pass craft work. This pass finds the same thing one level deeper. The DSP works. The state recall mostly works. The plugin loads. What's missing is the *vibe layer* — the parts of u-he and Baby Audio that make you want to keep the plugin in your template:

- Custom rotary that *looks* like b33p when you spot it on a strip.
- Mod matrix that visibly modulates.
- Preset browser that audits while you browse.
- A/B button for "is that actually better?"
- Knobs that respond to shift-drag like every other synth made since 2010.

None of these are big DSP work. They're craft work. They're also the difference between "indie plugin" and "the plugin a producer keeps."

