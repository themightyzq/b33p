# Using b33p

This is the long-form user guide. The README has the elevator pitch and a quickstart; this file goes deeper. Everything here also lives in the in-app **About b33p** dialog (`Cmd+/` or **Help → About b33p**) in condensed form.

## The mental model in one paragraph

b33p is built around a four-lane pattern. Each lane is its own monophonic synth voice, and the voice editor (Oscillator / Amp Envelope / Filter / Effects / Master) edits whichever lane is currently selected. The "(Lane N)" suffix in the section titles tells you which lane is being edited. Click any lane in the pattern grid (or any of its events) to switch.

The Pitch Envelope is the only voice-shaped thing that's **not** per-lane — it's shared across all four voices, hence its "(shared across all lanes)" suffix.

Patterns are time-based, up to 10 seconds, and snap to a millisecond grid (10 ms ... 1000 ms, or off). Each event in a pattern carries its own start time, duration, pitch offset (semitones), and velocity (0..1). Velocity drives the visible height of the clip in the grid.

## Voice design

Pick a lane (click anywhere in it, or click an event), then tweak the voice editor. Every continuous knob has:

- A **dice button** (the small die icon to the right of each section's name or below each rotary) — rolls a single random value.
- A **lock button** (the padlock icon next to the dice) — pins the value across rolls. Locked params survive both per-knob dice rolls AND the section-level / project-level Randomize buttons.

The **Master Gain** is the one exception — it has neither dice nor lock. Random rolls of master gain are too good a way to blow ears or speakers, so it's simply excluded from every randomize entry point.

### Randomize Lane / Randomize All / per-knob dice

- The **Randomize Lane** button in the Master strip rolls every unlocked parameter on the currently-selected lane.
- The **Randomize All** button in the Pattern controls row (next to Export) rolls every unlocked parameter across all four lanes.
- Per-knob dice rolls just that knob.

All three respect locks and the safety caps on parameters that can produce inaudible / painful sounds when randomized:

| Parameter | Random cap | Slider cap |
| --- | --- | --- |
| Amp Attack | 500 ms | 5 s |
| Amp Release | 100 ms | 5 s |
| Filter Resonance Q | 5 | 20 |
| Distortion Drive | 20 | 100 |

Manual editing always has the full slider range. Only random rolls are clamped.

### Custom oscillator

Pick **Custom** from the Waveform dropdown. An **Edit...** button appears. Click it to open the popup waveform editor — a horizontal canvas where you click + drag to draw a single-cycle waveform. The default starting shape is a sine wave; drawing replaces it. The shape is per-lane and saved in the `.beep` file alongside everything else.

The pitch envelope still applies to a custom waveform — the oscillator just substitutes the drawn shape for the usual sin / square / saw / etc.

## Pattern editing

### Mouse gestures

- **Drag in empty grid area** — draw a new event whose duration matches the dragged distance.
- **Double-click in empty grid area** — drop an event at the default 100 ms duration.
- **Drag a clip body** — move horizontally; vertical drag retargets the lane.
- **Drag a clip's left edge** — resize from the left (the right edge stays anchored).
- **Drag a clip's right edge** — resize from the right.
- **Drag a clip's top edge** — set the event's velocity. Top of the lane = velocity 1.0; centre of the lane = 0.
- **Click in the grid's top ruler row** — park the playhead at that time. `Cmd+V` then pastes the clipboard there.
- **Right-click on a clip** — Delete or Duplicate.
- **Right-click on empty lane area** — Generate Random Pattern in this lane / Clear all events in this lane (also reachable from the **Lane** menu).
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
| `Cmd + /` | About b33p |

On Windows / Linux, swap `Cmd` for `Ctrl`.

### Multi-select + clipboard

- Shift-click a clip to add it to the selection (or remove it).
- A drag on any selected clip moves the entire group by the same delta. Vertical lane-retarget only applies when one event is selected (otherwise everything would stack).
- Resize gestures (left / right / top edges) operate on the primary clip only — the most-recently-clicked one.
- Cmd+C captures the selection with timing relative to the earliest event. Cmd+V drops the captured group at the current playhead, preserving the relative start times and lane.

### Generate Random Pattern

Right-click any empty area in a lane and pick **Generate Random Pattern in this lane** (or use **Lane → Generate Random Pattern in Lane N** from the menu bar). Drops 4–8 events with random positions snapped to the current grid (or free if grid is "Off"), durations 50–200 ms (or 1–3 grid steps if a grid is set), velocities 0.5–1.0. Pitch offsets stay at 0 — use Randomize Lane afterwards to also roll the voice settings.

Each generation is a single undoable transaction (`Cmd+Z` reverts the whole batch).

## Saving and loading

Projects save as `.beep` files. The format is versioned (currently v2) — older `.beep` files always open in newer b33p versions via explicit migrations. v1 files (anything saved with v0.1.0) load by fanning the single voice's parameters out to all four lanes, so the project sounds identical on reload.

Closing the app or starting a New project while you have unsaved changes prompts Save / Discard / Cancel. Cancel keeps the current project open; Discard wipes; Save runs the normal save flow (and only proceeds if the save succeeds).

## Audio settings

If you don't hear anything, the most likely cause is that b33p picked a different output device than you expected. **File → Audio Settings...** opens JUCE's standard device picker — pick your output device, set the sample rate / buffer size, and the change applies live (you can leave the dialog open while testing).

## Exporting WAV

Click the **Export...** button in the Pattern controls (right of the grid combo). Pick destination, sample rate (8 / 11.025 / 16 / 22.05 / 44.1 / 48 kHz), bit depth (8 / 16 / 24-bit), and mono / stereo. The render runs offline in a background thread with a progress window — for typical patterns it's sub-second.

## Things that are intentionally not here (yet)

See the post-MVP roadmap in [TODO.md](../TODO.md) for the canonical list. Briefly:

- No tempo / BPM concept — patterns are seconds, not bars/beats. Musical timing is on the post-MVP list.
- No MIDI input. b33p only auditions / plays its internal pattern.
- No plugin (VST3 / AU) build yet — standalone only.
- No preset browser — the `.beep` file IS the preset.
- One pitch envelope shared across lanes (per-lane curves are post-MVP).
