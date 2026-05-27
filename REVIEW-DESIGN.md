# b33p — Craft / Design Review (2026-05-27)

Visual + interaction craft pass on the running standalone (macOS, default window, 1502×1007). JUCE desktop synth with a custom flat-dark LookAndFeel and per-lane accent colours — web-specific checks (DOM/ARIA, breakpoints, browser zoom, framework defaults) are N/A and skipped. Cold-stranger / journey findings live in `REVIEW-USER.md`; this is purely "does it look and feel made-with-care."

Method note: states triggered by live interaction (hover, focus, drag preview, error dialogs) couldn't be driven through JUCE controls via automation, so those are flagged as observed-from-static or unverified where relevant.

---

## Critical

### Modulation Slot 4 is clipped by the section's bottom edge
- **What:** In the Modulation section, the 4th matrix slot renders only its top half — the Source/Destination combos are sliced to ~half height and the amount readout is garbled (renders as "1.4x07x"). Slots 1–3 render full-height and clean; slot 4 is bisected by the section border. Not content-dependent — the 4th row simply doesn't fit.
- **Where:** Modulation section, `Source/UI/ModulationSection.cpp` `resized()` (LFO row 70px + dice/lock row + hint 16px + 4 × ~26px slot rows exceed the column-packed ~220px section height).
- **Why it matters:** A core control on the default screen looks broken/unfinished on every launch — the single loudest "vibe-coded" tell in the app.

### Empty-pattern hint renders mojibake "â€" instead of an em-dash
- **What:** The empty grid's hint reads "Drag in a lane to draw a beep **â€** or double-click for default size." — a corrupted em-dash. (The playhead readout's "—" a few px away renders correctly, so it's isolated to this string.)
- **Where:** `Source/UI/PatternGrid.cpp:758`. The literal holds a valid UTF-8 em-dash (`e2 80 94`) that JUCE paints as Latin-1.
- **Why it matters:** The first text a user sees on an empty grid is visibly garbage. Fix: use ASCII `-` or wrap the string in `juce::CharPointer_UTF8(...)` (the pattern already used elsewhere to dodge this exact issue).

---

## High Impact

### Same knob, wildly different sizes between sections
- **What:** The Oscillator's lone Pitch knob is roughly 2.5× the diameter of the Amp Envelope's Attack/Decay/Sustain/Release knobs sitting right beside it. A rotary should read as one consistent control; here size tracks "how much empty space is in the section," not meaning.
- **Where:** top row — `OscillatorSection` (sine/basic modes, one knob) vs `AmpEnvSection` (four knobs).
- **Why it matters:** Breaks the visual rhythm across the editor and makes the sparse sections look unfinished next to the dense ones.

### Numeric readouts show raw float precision, then truncate with "…"
- **What:** Modulation amount readouts show "0.29007…", "0.45395…", "0.71934…"; the Scope readout shows "1.000…". Five decimals, clipped mid-number.
- **Where:** `ModulationSection` matrix amount sliders; `PatternSection` Randomization-Scope readout.
- **Why it matters:** Truncated long decimals read as unfinished and convey nothing usable. Fix: format to 2 decimals (bipolar `+0.29` for the matrix amount, which is −1..1), and size the readout so it never needs "…".

### Per-knob dice + lock icons are near-invisible
- **What:** Every knob carries a dice (randomize) + lock pair below its value, drawn as faint gray glyphs on a dark button — below the ~3:1 contrast bar for UI components. A headline feature (per-knob dice/lock) is barely discoverable. Note the tension: at correct contrast, ~30 glyphs across the editor would also become visual noise.
- **Where:** every `LabeledSlider` randomizer pair (Effects, Filter, Mod FX, Modulation, Oscillator…).
- **Why it matters:** A core differentiator is both hard to find and, if naively brightened, would clutter. Fix: raise resting contrast to ≥3:1 *and* de-emphasize until hover/selection (hover-reveal the pair, keep a quiet resting dot), so it's findable without becoming noise.

### Focus state is unverified and likely invisible on the flat dark theme
- **What:** Couldn't confirm a visible focus indicator on any control; the custom flat `B33pLookAndFeel` doesn't appear to draw one, and JUCE's default focus ring is typically invisible against a dark flat background.
- **Where:** `Source/UI/B33pLookAndFeel.{h,cpp}` — focus drawing for sliders/buttons/combos.
- **Why it matters:** Keyboard users (the app documents Tab-able controls + grid shortcuts) can't see what's focused. Fix: draw a deliberate accent-coloured focus ring/underline in the LookAndFeel and confirm tab order matches visual order. *(Unverified via static capture — confirm with a keyboard pass.)*

---

## Nice to Have

### Lone Pitch knob leaves a large dead zone
- **What:** Because the Oscillator section (in basic/sine modes) has one knob stretched huge (see knob-size finding), most of the section is empty space while its neighbours are packed.
- **Where:** `OscillatorSection`.
- **Why it matters:** Reinforces the density mismatch. Fix: ties to the consistent-knob-size fix — cap the knob and top-align the cell rather than centring one giant knob.

### Preset nav uses text "<" / ">" while every other compact control uses drawn icons
- **What:** The Master section's preset prev/next are literal `<` and `>` text buttons, whereas dice, lock, and combo carets are drawn glyphs.
- **Where:** Master section preset nav (`MasterSection`).
- **Why it matters:** Minor iconography inconsistency. Fix: use the icon set's chevrons.

### Two stacked empty-state messages in the pattern area
- **What:** The empty grid shows a 2-line gesture hint centred in the lanes *and* a separate "Select a clip to edit lane, start, duration, pitch, and velocity." inspector hint below — two greyed messages competing in one empty view.
- **Where:** pattern grid empty state + inspector strip empty state.
- **Why it matters:** Mild redundancy/visual competition in the empty state. Fix: suppress the inspector hint until at least one clip exists, or merge into a single onboarding line.
