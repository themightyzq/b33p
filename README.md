# b33p

> Small sounds. Fast iteration. Infinite dice rolls.

**b33p** is a focused sound-design tool for short synthesized beeps, blips, alarms, and droid chatter — the kind of sounds you'd hear from Star Wars droids, retro games, sci-fi UIs, and chirpy gadgets. Design a voice, lay it out on a pattern timeline, hit Randomize until something surprises you, then export a WAV. Or load it as a VST3 / AU and play it from a MIDI keyboard.

Built for sound designers, game developers, and synth hobbyists who want a tight workflow for one specific problem: making a lot of good tiny sounds, quickly.

## Status

**Active development.** The post-MVP feature set has landed: full modulation engine, multi-mode oscillator and filter, per-event overrides, presets, MIDI input, polyphony, plugin formats. See [TODO.md](TODO.md) for what remains (installers, AAX, IMA-ADPCM). Built binaries for macOS, Linux, and Windows are attached to each [GitHub Actions run on `main`](https://github.com/themightyzq/b33p/actions/workflows/build.yml).

## Features

### Sound design
- **Per-lane voice design** — 4 lanes, 4 independently-tuneable timbres. Pick a lane in the pattern grid; the voice editor retargets to that lane's parameters.
- **Multi-mode oscillator** — sine, square, triangle, saw, noise, **drawable custom single-cycle waveform**, **wavetable** (4 morphable slots per lane), **FM** (two-operator sine), **ring modulation** (sine carrier × sine modulator + wet/dry).
- **Drawn pitch envelope** — click in the curve to add points, drag to shape, right-click to delete. Shared across lanes.
- **ADSR amp envelope** with a live visualiser.
- **Multi-mode filter** — Lowpass / Highpass / Bandpass (12 dB/oct biquad), feedback Comb, Formant (5-vowel interpolation A/E/I/O/U). Live response visualiser plots whichever mode you're in.
- **Bitcrush + distortion** in the dry chain.
- **Modulation effect slot** at the end of the chain — Chorus / Reverb / Delay / Flanger / Phaser, three per-mode parameters.
- **LFOs and modulation matrix** — 2 free-running LFOs per lane (Sine / Triangle / Saw / Square, 0..30 Hz), 4 matrix slots routing either LFO to any of 11 voice destinations with a bipolar amount.
- **Per-event parameter overrides** — pin up to 4 voice parameters per pattern hit. Right-click a clip → *Edit overrides...*.
- **Per-event probability / ratcheting / humanize** — every event carries a 0..1 probability, a 1..8 ratchet count, and a 0..1 humanize amount.

### Pattern + playback
- **Pattern sequencer** — 4 lanes, up to 10 seconds, per-event start / duration / pitch offset / velocity.
- **Tempo + time signature** — 20..999 BPM, 4/4 / 3/4 / 6/8 / 7/8 / 9/8 / 12/8 / 5/4 / 6/4 / 2/4. Drives both the musical-snap grid and the bars/beats time display.
- **Musical or time-based snap** — pick from 1/32 note ... Whole (BPM-derived) or fixed milliseconds.
- **Dual-time ruler** — seconds along the top, bars/beats along the bottom; the playhead readout shows both.
- **Direct-manipulation clip editing** — drag the body to move (vertically across lanes), drag either edge to resize, drag the top edge to set velocity, double-click empty grid to drop at default size, click + drag empty grid to draw any size. Cursor changes to telegraph the gesture.
- **Multi-select + clipboard** — shift-click to extend the selection, Cmd+A select all, Cmd+C / Cmd+V copy/paste at the playhead.
- **Generate random pattern** — right-click an empty lane area for instant rhythm.
- **MIDI input** — play the selected lane from any connected keyboard. 8-voice polyphonic pool so chords ring together.

### Randomization
- **Per-knob dice + lock** on every parameter (except master gain — random rolls there are too good a way to blow ears).
- **Randomization Scope slider** — 0.05..1.0 multiplier on every roll. 1.0 = full range; smaller values cluster rolls around the current value for subtle variation.
- **Randomize Lane / Randomize All** entry points respect locks and the safety caps on attack / release / resonance / drive.

### Presets + project state
- **Preset browser** — Save Preset / Browse Presets in the File menu. Per-user presets directory under `~/Library/Application Support/b33p/Presets` (or platform equivalent).
- **Factory presets** — 4 starter patches ship with the app and seed the presets directory on first launch (FM Bell, Resonant Stab, Delay Pad, Ring Mod Robot).
- **Project save / load** — `.beep` files carry the full project state. Format is versioned (currently v11) with explicit forward-only migrations; older `.beep` files always open in newer b33p versions.
- **Recent Files submenu** in File.
- **Full undo / redo** through every editing surface.

### Render + plugin builds
- **Audio export** — WAV / AIFF / FLAC / OGG Vorbis at 8 / 11.025 / 16 / 22.05 / 44.1 / 48 / 88.2 / 96 kHz, 8 / 16 / 24-bit, mono or stereo.
- **Batch export** — render N dice-rolled variations into numbered files in one shot. APVTS state is restored after the batch so the patch survives.
- **VST3 + AU plugin builds** alongside the standalone .app — same source tree, single CMake target.

## Screenshots

![b33p main window — voice editor, pitch envelope, and pattern sequencer](docs/images/hero.png)

The full editor at default state: per-lane voice editor (Oscillator / Amp Envelope / Filter / Effects / Modulation Effects / Modulation / Master) with a "(Lane N)" suffix telling you which lane you're editing, the shared drawable pitch envelope, and the pattern sequencer with per-lane name + mute + solo, snap grid, BPM + time-signature controls, and Randomize-All / Export controls.

> *Screenshot is from v0.1.0 — a refresh covering today's UI is pending.*

## Installation

Prebuilt binaries: grab the latest green CI run from [GitHub Actions](https://github.com/themightyzq/b33p/actions/workflows/build.yml) and download the `b33p-macos-latest`, `b33p-ubuntu-latest`, or `b33p-windows-latest` artifact. Or build from source:

### Prerequisites (all platforms)

- CMake 3.22+
- A C++17 compiler
- Git

### macOS

```sh
git clone https://github.com/themightyzq/b33p.git
cd b33p
cmake -B build -G Xcode
cmake --build build --config Release
```

The output bundles land at:

- Standalone : `build/B33p_artefacts/Release/Standalone/b33p.app`
- VST3       : `build/B33p_artefacts/Release/VST3/b33p.vst3`
- AU         : `build/B33p_artefacts/Release/AU/b33p.component`

### Windows

```powershell
git clone https://github.com/themightyzq/b33p.git
cd b33p
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

The output bundles land at:

- Standalone : `build\B33p_artefacts\Release\Standalone\b33p.exe`
- VST3       : `build\B33p_artefacts\Release\VST3\b33p.vst3`

(AU is macOS-only and is auto-skipped on non-Apple builds.)

### Linux

```sh
sudo apt install libasound2-dev libjack-jackd2-dev libcurl4-openssl-dev \
  libfreetype6-dev libfontconfig1-dev libx11-dev libxcomposite-dev \
  libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \
  libglu1-mesa-dev mesa-common-dev
git clone https://github.com/themightyzq/b33p.git
cd b33p
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The output bundles land at:

- Standalone : `build/B33p_artefacts/Release/Standalone/b33p`
- VST3       : `build/B33p_artefacts/Release/VST3/b33p.vst3`

### Installing the plugin formats

Copy the freshly-built bundles into the standard host search paths. On macOS:

```sh
# VST3
cp -r build/B33p_artefacts/Release/VST3/b33p.vst3 ~/Library/Audio/Plug-Ins/VST3/

# AU
cp -r build/B33p_artefacts/Release/AU/b33p.component ~/Library/Audio/Plug-Ins/Components/
```

Most DAWs scan these directories on launch; some (Logic, Ableton) will need to re-validate plugins before b33p shows up in the synth list.

For the long-form user guide — every gesture, every keyboard shortcut, the per-lane voice model in depth, modulation engine, presets, MIDI workflow, plugin host usage — see **[docs/USAGE.md](docs/USAGE.md)**.

## Quick start — your first beep

1. Open b33p (standalone) or instantiate the VST3 / AU in your DAW. The voice editor is on the left + middle; the pattern grid is at the bottom.
2. Click the **Audition** button (or press **Shift+Space**) to hear the default voice. Or play notes from a connected MIDI keyboard.
3. Click **File → Browse Presets** to start from one of the factory presets, or click **Randomize Lane** in the Master strip to roll a fresh patch.
4. In the **Pattern** grid at the bottom, drag in a lane to draw an event (or double-click for default size). Drag the body to move, drag the edges to resize, drag the top edge to set velocity, hit Delete to remove.
5. Press **Space** to start / stop pattern playback. Edits are audible immediately while looping.
6. Click **Export...** in the Pattern controls to render a WAV (or set Variations > 1 for batch export).

## Keyboard shortcuts

### Transport
| Shortcut | Action |
| --- | --- |
| `Space` | Play / stop the pattern |
| `Shift + Space` | Audition the selected lane's voice |

### File / Edit
| Shortcut | Action |
| --- | --- |
| `Cmd + N` | New project |
| `Cmd + O` | Open project |
| `Cmd + S` | Save |
| `Cmd + Shift + S` | Save As… |
| `Cmd + Z` | Undo |
| `Cmd + Shift + Z` | Redo |
| `Cmd + /` | About b33p |

### Pattern editing (when the pattern grid has focus)
| Shortcut | Action |
| --- | --- |
| `Cmd + A` | Select every event |
| `Cmd + C` | Copy the selected events |
| `Cmd + V` | Paste the clipboard at the playhead |
| `Delete` / `Backspace` | Delete the selected events |
| `←` / `→` | Nudge the selected events by one grid step |
| `Shift + ←` / `Shift + →` | Nudge by ten grid steps |
| `Esc` | Deselect everything |

On Windows / Linux, swap `Cmd` for `Ctrl`.

## Mouse / right-click

- **Drag in empty grid area** — draw a new event whose duration matches the dragged distance.
- **Double-click in empty grid area** — create an event at the default 100 ms duration.
- **Drag a clip body** — move horizontally; vertical drag retargets the lane.
- **Drag a clip's left or right edge** — resize.
- **Drag a clip's top edge** — set the event's velocity.
- **Click in the grid's ruler row (top strip)** — park the playhead at that time. Cmd+V then pastes the clipboard there.
- **Right-click on a clip** — Delete / Duplicate / Edit overrides...
- **Right-click on empty lane area** — Generate random pattern / Clear lane.
- **Double-click on a lane label (the "1" / "2" / ...)** — rename the lane (persists in the `.beep` file).

## File format

Projects save as `.beep` files. A `.beep` is a versioned `ValueTree` serialization of the full project: per-lane voice parameters (oscillator + envelopes + filter + effects + modulation + LFOs + matrix), pitch-envelope curve, pattern (lanes, events, BPM, time signature, lane meta — names / mute / solo, custom waveforms, per-event overrides, probability / ratchets / humanize), and randomizer locks. It's self-contained — no external sample files — so a `.beep` is portable and small.

The format is versioned (currently v11) with explicit forward-only migrations; older `.beep` files always open in newer b33p versions and are upgraded silently on load.

## Contributing

Issues are welcome — bug reports, feature requests, rough-edge sightings.

Pull requests: please open an issue first to discuss scope. The project has tight focus (see [TODO.md](TODO.md)), and PRs for items not on the active roadmap will be redirected rather than merged.

## License

Released under **GPL-3.0-or-later**. See `LICENSE` for the full text.

In short: you can use, modify, and redistribute b33p freely, but derivative works must also be GPL-3.0-or-later and distributed with source.

## Credits

- **b33p** by **ZQ SFX** — [github.com/themightyzq](https://github.com/themightyzq)
- Built with [JUCE](https://juce.com/)
- Tested with [Catch2](https://github.com/catchorg/Catch2)

---

For the full roadmap, see [TODO.md](TODO.md).
