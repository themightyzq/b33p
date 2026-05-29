# Using b33p

This is the long-form user guide. The README has the elevator pitch and a quickstart; this file goes deeper. A condensed shortcut reference also lives in the in-app **Help → Keyboard Shortcuts…** dialog, and credits/version in **Help → About b33p**.

## The mental model in one paragraph

b33p is built around a four-lane pattern. Each lane is its own monophonic synth voice, and the voice editor (Oscillator / Amp Envelope / Filter / Effects / Master / Mod FX / Modulation) edits whichever lane is currently selected. The "(Lane N)" suffix on the **Oscillator** section title — and the lane-coloured accent strip carried across every section — tells you which lane is being edited. Click any lane in the pattern grid (or any of its events) to switch; the whole voice editor re-tints to that lane's colour.

The Pitch Envelope is the only voice-shaped thing that's **not** per-lane — it's shared across all four voices, hence its "(shared across all lanes)" suffix.

Patterns are time-based — under the hood, every event carries a start time and duration in seconds. Tempo and time signature are first-class fields though, so the snap grid can be a beat fraction (1/16 note, 1/8 note, …) and the ruler displays bars/beats alongside seconds. Each event in a pattern carries its own start time, duration, pitch offset (semitones), velocity (0..1), probability (0..1), ratchet count (1..8), humanize amount (0..1), and up to 4 per-hit parameter overrides.

## Voice design

Pick a lane (click anywhere in it, or click an event), then tweak the voice editor. Every continuous knob has:

- A **dice button** (the small die icon to the right of each section's name or below each rotary) — rolls a single random value.
- A **lock button** (the padlock icon next to the dice) — pins the value across rolls. Locked params survive both per-knob dice rolls AND the section-level / project-level Randomize buttons.

The **Master Gain** is the one exception — it has neither dice nor lock. Random rolls of master gain are too good a way to blow ears or speakers, so it's simply excluded from every randomize entry point.

Every knob also responds to the standard synth gestures:

- **Drag** to adjust; **hold Shift before dragging** for fine (≈6×) adjustment.
- **Mouse-wheel / two-finger scroll** over a knob steps the value.
- **Double-click** resets the knob to the parameter's default.
- **Right-click** (or Ctrl-click) opens a menu: *Enter value…* (type an exact number into the readout) and *Reset to default*.

Value readouts use a monospaced face so digits don't jitter as you drag.

### Oscillator modes

Pick the mode from the Waveform combo at the top of the Oscillator section. Different modes expose different sliders below the combo — the unused ones hide so the row stays tidy.

| Mode      | What it does | Per-mode controls |
| --------- | --- | --- |
| Sine / Square / Triangle / Saw | The classic four. Naive (non-band-limited) oscillator math. | Pitch only |
| Noise     | Uniform random noise per sample. | Pitch (no audible effect) |
| Custom    | Plays a single drawn cycle (256 samples). Click **Edit slots...** to open the editor. | Pitch only |
| Wavetable | Linearly interpolates between **four** drawn slots based on a 0..1 morph parameter. Click **Edit slots...** to open the editor; the Morph slider blends between adjacent slots. | Pitch + Morph |
| FM        | Two-operator sine FM. The carrier runs at the base pitch; the modulator runs at `pitch * ratio` and offsets the carrier's instantaneous phase by `depth * sin(modulator)`. | Pitch + FM Ratio + FM Depth |
| Ring      | Sine carrier × sine modulator (modulator at `pitch * ratio`), then crossfaded with the dry carrier. | Pitch + Ring Ratio + Ring Mix |

### Custom + wavetable editor

The drawing canvas is the same dialog for both Custom and Wavetable modes — opened by **Edit slots...** (or by re-clicking after closing). Four "Slot 1..4" tabs across the top pick which slot you're drawing. Click + drag to draw values across the cycle.

- **Custom mode** plays Slot 1 only — Slot 2..4 are inert but still saved.
- **Wavetable mode** uses all four. The Morph slider's 0..1 range maps onto the slot positions: 0 = pure Slot 1, 0.33 = blend of 1 ↔ 2, 0.66 = blend of 2 ↔ 3, 1.0 = pure Slot 4.
- Slots seed with a sine cycle the first time the mode is selected so playback is never silent before drawing. Drawing replaces the seed.

### Multi-mode filter

The Filter section's Type combo picks Lowpass / Highpass / Bandpass / Comb / Formant. The response visualiser above the combo plots the active type's analytic magnitude curve, log-frequency on x, dB on y. The two sliders below the combo carry mode-dependent semantics:

| Mode | Cutoff means | Resonance means | Vowel means |
| ---- | --- | --- | --- |
| LP / HP / BP | corner / centre frequency | Q (peak emphasis or BP bandwidth) | unused |
| Comb | comb peak fundamental (delay = 1 / cutoff) | feedback gain (mapped onto 0..0.95) | unused |
| Formant | unused | unused | 0 = vowel A, 1 = vowel U with E / I / O at quarter steps |

In Formant mode the Cutoff / Resonance sliders disappear and a single Vowel slider takes their place.

### Bitcrush, distortion, modulation effects

- **Bitcrush** lives in the dry chain after the filter. Bit Depth (1..16) and Crush Sample Rate (1 kHz..48 kHz, log-skewed) — drop both for grimey 8-bit textures.
- **Distortion** sits after bitcrush. Drive (0.1..100) feeds a soft clipper; rolls are capped at 20 for randomization safety.
- **Mod FX** is a single wet-effect slot at the end of the chain (Chorus / Reverb / Delay / Flanger / Phaser / None). The three normalised P1 / P2 / Mix sliders re-label themselves per type:

  | Type    | P1     | P2          | Mix     |
  | ------- | ------ | ----------- | ------- |
  | Chorus  | Rate   | Depth       | Wet/dry |
  | Reverb  | Size   | Damping     | Wet     |
  | Delay   | Time   | Feedback    | Wet/dry |
  | Flanger | Rate   | Depth       | Wet/dry |
  | Phaser  | Rate   | Depth       | Wet/dry |
  | None    | (greyed) | (greyed) | (greyed) — bypass |

### LFOs and modulation matrix

The **Modulation** section (bottom-left of the voice editor, beside the Pitch Envelope) hosts:

- **Two LFO panels** at the top — Shape (Sine / Triangle / Saw / Square) and Rate (0..30 Hz). LFOs are free-running — they don't retrigger on note events, so a slow filter sweep persists across many notes.
- **Four matrix slots** below — each slot picks a Source (None / LFO 1 / LFO 2), a Destination (one of 11 voice parameters), and a bipolar Amount (-1..+1).

Modulation is evaluated per audio block. A slot with non-None source + dest contributes `lfo_output × amount × 0.5` to the destination's normalised value. Two slots on the same destination stack, so you can drive a parameter through its full range by routing both LFOs to it.

### Randomization Scope and the dice path

Four randomize entry points:

- **Randomize All** (Master strip) rolls every unlocked parameter on the currently-selected lane.
- **Randomize Params** (Pattern controls row) rolls voice parameters for every lane that currently holds clips — randomizing a silent lane's voice is wasted motion. Empty pattern falls back to rolling all four lanes.
- **Randomize Pattern** (Pattern controls row) scatters fresh random clips across all four lanes in one undoable step. Doesn't touch voice parameters.
- **Per-knob dice** rolls just that knob.

The **Scope** slider in the Pattern controls row scales how wild the rolls get:

- **Scope = 1.0** — full range (legacy behaviour). Each roll picks any value in the parameter's range.
- **Scope < 1.0** — rolls are constrained to a window centred on the current value. Scope 0.1 = ±5% of the parameter's normalised range. Use it when you've found a good patch and want gentle variations.

All roll paths respect locks and the safety caps on parameters that can produce inaudible / painful sounds when randomized:

| Parameter | Random cap | Slider cap |
| --- | --- | --- |
| Amp Attack | 500 ms | 5 s |
| Amp Release | 100 ms | 5 s |
| Filter Resonance Q | 5 | 20 |
| Distortion Drive | 20 | 100 |

Manual editing always has the full slider range. Only random rolls are clamped.

The bulk Randomize All / Randomize Params buttons also run an **audibility guard** — after the roll, the synth offline-renders the audition for each touched lane and re-rolls level-critical parameters (amp env, filter cutoff/Q, Mod FX mix, FM depth, ring mix) if the result is near-silent, falling back to a known-audible safe patch if the re-rolls can't recover. So Randomize never produces a silent patch. Per-knob dice are left alone — they're deliberate single-parameter exploration.

When Randomize fires, every knob that just moved briefly flashes a brighter accent ring so you can see what changed.

### Seeing what's shaping the sound

When the synth is producing audio, knobs that are being actively modulated paint a soft accent halo whose intensity tracks the live modulation. Combined sources show through (max), so you'll see the strongest contributor at any instant.

- **Mod-matrix LFO routings** light their destination knob — turn the slot's amount up, watch the cutoff (or whichever destination) pulse at the LFO's rate. The mod-matrix slot's own indicator strip mirrors it.
- **The amp envelope** lights the **Gain** knob over the course of every note — you see attack rise, decay drop to sustain, sustain hold, release fade.
- **The pitch envelope** lights the **Pitch** knob whenever the drawn curve deflects from zero (intensity = `|semitones| / 12`, so an octave bend reads as a full glow).
- **The Mod FX section** — when the type is Chorus, Flanger, or Phaser, the section's three knobs pulse at the effect's actual rate (the warble's source, made visible). None / Reverb / Delay don't pulse.
- **The Amp Envelope visualizer** also paints a vertical playhead that walks left-to-right through the A/D/S/R curve as the note progresses, so you can see exactly where the envelope is.

All of these freeze the moment playback ends — nothing in the UI animates while the synth is silent, so motion is reserved for "this is shaping the sound you're hearing right now."

## Pattern editing

### Mouse gestures

- **Drag in empty grid area** — draw a new event whose duration matches the dragged distance.
- **Double-click in empty grid area** — drop an event at the default 100 ms duration.
- **Drag a clip body** — move horizontally; vertical drag retargets the lane.
- **Drag a clip's left edge** — resize from the left (the right edge stays anchored).
- **Drag a clip's right edge** — resize from the right.
- **Drag a clip's top edge** — set the event's velocity. Top of the lane = velocity 1.0; centre of the lane = 0.
- **Click in the grid's top ruler row** — park the playhead at that time. `Cmd+V` then pastes the clipboard there.
- **Right-click on a clip** — Delete / Duplicate / Edit overrides…
- **Right-click on empty lane area** — Generate Random Pattern / Clear all events (also reachable from the **Lane** menu).
- **Double-click on a lane label** (the "1" / "2" / ... at the left) — rename the lane. Names are persisted in the `.beep`.
- **Click the M / S buttons** next to a lane label — Mute / Solo. Solo wins over mute if both are on; while any lane is soloed, only soloed lanes play.

### Keyboard shortcuts

When the pattern grid has focus (click in it or on an event):

| Shortcut | Action |
| --- | --- |
| `Space` | Play / stop the pattern |
| `Shift + Space` | Audition the selected lane's voice once |
| `Cmd + A` | Select every event in the pattern |
| `Cmd + C` | Copy the selected events |
| `Cmd + V` | Paste the clipboard at the playhead |
| `Delete` / `Backspace` | Delete the selected events |
| `←` / `→` | Nudge the selected events by one grid step |
| `Shift + ←` / `Shift + →` | Nudge by ten grid steps |
| `Esc` | Deselect everything |

Project / file shortcuts work anywhere in the window:

| Shortcut | Action |
| --- | --- |
| `Cmd + N` | New project |
| `Cmd + O` | Open project |
| `Cmd + S` | Save |
| `Cmd + Shift + S` | Save As… |
| `Cmd + Z` | Undo |
| `Cmd + Shift + Z` | Redo |

About b33p and the full shortcut reference are under the **Help** menu (Help → Keyboard Shortcuts… / About b33p).

On Windows / Linux, swap `Cmd` for `Ctrl`.

### Multi-select + clipboard

- Shift-click a clip to add it to the selection (or remove it).
- A drag on any selected clip moves the entire group by the same delta. Vertical lane-retarget only applies when one event is selected (otherwise everything would stack).
- Resize gestures (left / right / top edges) operate on the primary clip only — the most-recently-clicked one.
- Cmd+C captures the selection with timing relative to the earliest event. Cmd+V drops the captured group at the current playhead, preserving the relative start times and lane.

### Per-event properties (overrides + probability + ratcheting + humanize)

Right-click a clip and pick **Edit overrides...** to open the *Event Properties* dialog. Four override slots at the top let you pin specific voice parameters for that hit:

- Each slot has a Destination combo (None / Osc Pitch / Wavetable Morph / FM Depth / Ring Mix / Filter Cutoff / Filter Resonance / Distortion Drive / Mod FX P1 / Mod FX P2 / Mod FX Mix / Voice Gain) and a 0..1 normalised value slider.
- Overrides win outright over both the lane's APVTS base value and any LFO modulation on that destination — a clip can pin a parameter independently of any wiring.

Below the override slots, three more sliders carry per-event variability:

- **Probability** (0..1) — chance the event fires when its loop slot arrives. Rolled at snapshot time, so a roll-out skips the entire event (and any ratchets) until the next snapshot rebuild.
- **Ratchets** (1..8) — number of evenly-spaced retriggers within the event's duration. 1 disables. Higher counts split the event into N shorter hits at snapshot time.
- **Humanize** (0..1) — random jitter on start time (±25% of duration at full humanize) and velocity (one-sided softening — humanize never boosts loudness). Re-randomises each snapshot rebuild.

Apply commits all edits in one undo transaction. Cancel discards.

### Tempo, time signature, and snap

Three controls in the Pattern controls row drive musical timing:

- **BPM** (20..999) — pattern tempo in beats per minute.
- **Sig** combo — time signature (4/4, 3/4, 6/8, 7/8, 9/8, 12/8, 5/4, 6/4, 2/4).
- **Grid** combo — snap interval. Time-based options (10 ms .. 1000 ms) coexist with musical options (1/32 note .. Whole). Musical entries auto-update when BPM changes.

The pattern grid's ruler shows seconds along the top and bar/beat ticks along the bottom. Playhead readout reads `Bar 2.3 — 0.95 / 5.00s` — bar.beat alongside seconds, so musical and time-domain users can both see what's happening at a glance.

### Generate Random Pattern

Right-click any empty area in a lane and pick **Generate Random Pattern in this lane** (or use **Edit ▸ Lane ▸ Generate Random Pattern in Lane N** from the menu bar). Drops 4–8 events with random positions snapped to the current grid (or free if grid is "Off"), durations 50–200 ms (or 1–3 grid steps if a grid is set), velocities 0.5–1.0. Pitch offsets stay at 0 — use **Randomize All** (Master section) afterwards to also roll the voice settings.

The pattern toolbar has the all-lanes equivalent: **Randomize Pattern** scatters fresh random clips across every lane in one undoable step.

Each generation is a single undoable transaction (`Cmd+Z` reverts the whole batch).

## MIDI input

Every connected MIDI input device routes notes into the currently-selected lane's voice. MIDI note 60 (middle C) is the no-transposition reference — higher notes pitch up, lower notes pitch down in semitones. Velocity is taken from the MIDI byte (0..127 → 0..1).

The MIDI voice pool is **8 voices wide**, separate from the 4 pattern lane voices, so chords ring together up to 8 notes deep without fighting the pattern playback. Note-on round-robins through the pool; note-off looks up the matching voice and releases its amp envelope.

To pick a specific MIDI input device or to disable MIDI input, use your host's MIDI settings (when running as a plugin) or the Standalone wrapper's Options menu (when running as the .app).

## Presets

Three menu items under **File** drive the preset workflow:

- **Save Preset...** — prompts for a name and writes the current state to `<name>.beep` in the per-user presets directory. If a preset with that name already exists, it asks before overwriting.
- **Browse Presets...** — opens a list of every `.beep` file in the presets directory. Double-click a row (or pick it and click **Load**) to apply. **Delete** removes a preset (with a confirm prompt). Loading a preset is a single undoable step — `Cmd+Z` restores the previous patch.
- **Restore Factory Presets...** — rewrites the four shipped factory patches to their originals (after a confirm), recovering any you've saved over. Your own presets are untouched.

To move a single voice between lanes or between two b33p instances, use **Edit ▸ Lane ▸ Copy *Lane N* voice (clipboard)** and **Paste voice into *Lane N* (clipboard)** — it carries every parameter for the lane plus its wavetable slots through the system clipboard. Paste is one undoable step.

The presets directory is platform-specific:

- macOS  : `~/Library/Application Support/b33p/Presets`
- Windows: `%APPDATA%\b33p\Presets`
- Linux  : `~/.config/b33p/Presets`

Loading a preset routes through the same dirty-prompt as File ▸ Open, so you can't lose unsaved work by clicking a preset.

### Factory presets

The first time the app launches, the presets directory is seeded with four starter patches that demonstrate the synth's range:

- **Factory - FM Bell** — FM mode at ratio 2.5, depth 6 — the classic inharmonic bell timbre.
- **Factory - Resonant Stab** — Saw + LP with Q=12, LFO 1 sweeping the cutoff via the modulation matrix.
- **Factory - Delay Pad** — Triangle with slow ADSR + Mod-FX Delay at 0.85 feedback / 0.55 mix.
- **Factory - Ring Mod Robot** — Ring mode at 3.7 ratio, tight envelope, 16 chatter hits with humanize=0.25.

Existing files are NEVER overwritten on subsequent launches — once a user tweaks "Factory - Delay Pad", their version wins forever. To get the originals back, use **File ▸ Restore Factory Presets...** (or delete the file and relaunch).

## Saving and loading

Projects save as `.beep` files. The format is versioned (currently v13) with explicit forward-only migrations; older `.beep` files always open in newer b33p versions and are upgraded silently on load. The migration chain covers v1 (single-voice MVP) through v13 (A/B compare state) — see `Source/State/ProjectState.h` for the full version history.

Closing the app or starting a New project while you have unsaved changes prompts Save / Discard / Cancel. Cancel keeps the current project open; Discard wipes; Save runs the normal save flow (and only proceeds if the save succeeds).

## Audio settings

When running as the **standalone .app**, JUCE's standalone wrapper exposes a device picker via the Options gear icon in the title bar — pick your output device, set the sample rate / buffer size, and the change applies live.

When running as a **VST3 / AU plugin**, audio routing is the host's responsibility — set up the synth track in your DAW as you would any other plugin instrument.

## Exporting audio

Click the **Export...** button in the Pattern controls (right of the grid combo). Pick:

- **Destination** — the file path; the extension is auto-corrected to match the chosen format.
- **Format** — WAV / AIFF / FLAC / OGG Vorbis.
- **Sample rate** — 8 / 11.025 / 16 / 22.05 / 44.1 / 48 / 88.2 / 96 kHz.
- **Bit depth** — 8 / 16 / 24 (OGG Vorbis ignores this — it picks its own).
- **Channels** — Mono or Stereo (mono content duplicated across both channels).
- **Variations** — 1 = single render; 2..100 = batch render N dice-rolled variations.

The render runs offline in a background thread with a progress window — for typical patterns it's sub-second.

### Batch export

Set Variations > 1 to render multiple dice-rolled takes in one shot. Variation 1 captures the user's current patch verbatim — they always have a clean reference; subsequent variations re-roll every unlocked parameter before rendering. APVTS state is snapshotted before the batch and restored after, so the user's patch survives the dice rolls.

Outputs are written to numbered siblings of the chosen path: `my_export.wav` with Variations=10 produces `my_export_001.wav` through `my_export_010.wav`, three-digit zero-padded so 100 variations sort naturally even in alphabetical listings.

## Plugin host usage

The same source tree builds **VST3** (Steinberg's modern format; loads in every major DAW), **AU** (Apple's Audio Units; macOS-only), and **Standalone**. From the build directory:

- VST3       : `build/B33p_artefacts/Release/VST3/b33p.vst3`
- AU         : `build/B33p_artefacts/Release/AU/b33p.component`
- Standalone : `build/B33p_artefacts/Release/Standalone/b33p.app` (or `.exe` / native binary)

Copy VST3 / AU bundles to the standard host search paths to make them discoverable:

```sh
# macOS
cp -r build/B33p_artefacts/Release/VST3/b33p.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -r build/B33p_artefacts/Release/AU/b33p.component ~/Library/Audio/Plug-Ins/Components/
```

Most DAWs scan these directories on launch; some (Logic, Ableton) will need to re-validate plugins before b33p shows up in the synth list.

Inside a host, b33p is a synth instrument:

- **MIDI in** drives the selected lane's voice (8-voice polyphony in the MIDI pool).
- **The pattern still plays** — you can run b33p's internal sequencer alongside the host's MIDI track if you want layered material.
- **Project state round-trips** through the host's session save — the host serialises the APVTS + ValueTree contents the same way b33p's `.beep` files do.

**DAW transport sync** is opt-in via the **Follow** toggle next to Play / Loop in the pattern controls. When on (and a host playhead is available), b33p's pattern playback mirrors host transport: host play / stop drives `playing`, and the playhead snaps to `host_time mod pattern_length` at the top of every block. Pattern BPM stays independent of host BPM — sound-design users routinely want a beep pattern at a different tempo than the host session. The toggle is inert in standalone mode (no host = no playhead) but its state is preserved in the `.beep` so a project saved standalone keeps its choice when reopened in a DAW.

**Host bypass freezes pattern playback.** Engaging the host's plugin-bypass on b33p stops audio generation entirely — the buffer is cleared and the pattern playhead is paused for as long as bypass is on. Any pattern events that would have fired during the bypass window are skipped (they don't queue up to fire on un-bypass). On un-bypass the output fades back in over ~10 ms so a long reverb or delay tail that was sounding at engage doesn't resume with a click. This is the standard instrument-plugin semantic; if you want the pattern to keep advancing while b33p is silent, mute the plugin's output bus in your host instead of using plugin-bypass.

## Things that are intentionally not here (yet)

See the post-MVP roadmap in [TODO.md](../TODO.md) for the canonical list. Briefly:

- **No per-lane pitch envelopes** — one pitch envelope is shared across all four voices.
- **No release installers / signed binaries** — binaries are distributed via the CI artifacts on each green push to `main`; signed installers aren't on the roadmap.
