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
