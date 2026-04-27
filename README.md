# b33p

> Small sounds. Fast iteration. Infinite dice rolls.

**b33p** is a focused sound-design tool for short synthesized beeps, blips, alarms, and droid chatter — the kind of sounds you'd hear from Star Wars droids, retro games, sci-fi UIs, and chirpy gadgets. Design a voice, lay it out on a pattern timeline, hit the dice until something surprises you, then export a WAV.

Built for sound designers, game developers, and synth hobbyists who want a tight workflow for one specific problem: making a lot of good tiny sounds, quickly.

## Status

**Pre-alpha — not yet released.** Active development toward v0.1 (MVP). See [TODO.md](TODO.md) for the roadmap.

## Features (MVP target)

What the first release will actually do:

- **Voice design** — one oscillator (sine, square, triangle, saw, noise), drawable pitch envelope, ADSR amp envelope, lowpass filter, bitcrush, distortion, master gain
- **Pattern sequencer** — up to 4 lanes, 10-second max, per-event pitch offset, grid snap (1/4 down to 1/32, or off)
- **Dice everywhere** — every parameter has a lock and a dice button. A top-level dice rerolls everything unlocked. Full undo support.
- **WAV export** — sample rates from 8 kHz up to 48 kHz, bit depths from 8-bit up to 24-bit, mono or stereo. Built for both clean renders and crunchy retro-console exports.
- **Project save / load** — `.beep` files carry the full project state

### Coming after v0.1

Roadmap highlights — not built yet, not promised on a date, but on the list. Full details in [TODO.md](TODO.md).

- **Custom drawn waveforms** (draw your own single-cycle shape)
- **Multiple voices per project** — one voice per lane
- **Generator presets** — one-click starting points for droids, alarms, weapons, UI
- **VST3 / AU plugin builds**
- **LFOs and a modulation matrix**
- **Wavetable, FM, ring-mod oscillators**
- **Preset browser with tags**

## Screenshots

_Screenshots live in `docs/images/`. Nothing to show yet — check back around v0.1._

## Installation

Prebuilt binaries: **TBD at v0.1.** Until then, build from source.

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

The app lands at `build/B33p_artefacts/Release/b33p.app`.

### Windows

```powershell
git clone https://github.com/themightyzq/b33p.git
cd b33p
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

The app lands at `build\B33p_artefacts\Release\b33p.exe`.

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

The app lands at `build/B33p_artefacts/b33p`.

## Quick start — your first beep

1. Open b33p. You land on the voice editor with a default patch.
2. Click the big **Dice** at the top. Do it a few times until something catches your ear. Click the lock next to any parameter you want to keep while rerolling the rest.
3. Switch to the **Pattern** tab. Click-drag in a lane to draw an event. Drag the edges to resize, drag the body to move, hit Delete to remove it.
4. Press **Space** to loop-play the pattern. Keep dicing and tweaking until it sounds right.
5. **File → Export WAV** to render.

## Keyboard shortcuts

| Shortcut | Action |
| --- | --- |
| `Space` | Audition voice / play pattern (context-dependent) |
| `Cmd/Ctrl + Z` | Undo |
| `Cmd/Ctrl + Shift + Z` | Redo |
| `Cmd/Ctrl + S` | Save project |
| `Cmd/Ctrl + Shift + S` | Save project as… |
| `Cmd/Ctrl + O` | Open project |
| `Cmd/Ctrl + E` | Export WAV |
| `Cmd/Ctrl + D` | Dice all unlocked |
| `Cmd/Ctrl + /` | About b33p |
| `Delete` / `Backspace` | Delete selected event |

## File format

Projects save as `.beep` files. A `.beep` is a versioned `ValueTree` serialization of the full project: voice parameters, pitch-envelope curve, pattern lanes and events, and lock/range state. It's self-contained — no external sample files — so a `.beep` is portable and small.

The format is versioned from v1 onward; older `.beep` files will always open in newer b33p versions via explicit migrations.

## Contributing

Issues are welcome — bug reports, feature requests, rough-edge sightings.

Pull requests: please open an issue first to discuss scope. The project has a strict MVP focus (see [TODO.md](TODO.md)), and PRs for out-of-phase features — however good — will be redirected to the roadmap rather than merged.

## License

Released under **GPL-3.0-or-later**. See `LICENSE` for the full text.

In short: you can use, modify, and redistribute b33p freely, but derivative works must also be GPL-3.0-or-later and distributed with source.

## Credits

- **b33p** by **ZQ SFX** — [github.com/themightyzq](https://github.com/themightyzq)
- Built with [JUCE](https://juce.com/)
- Tested with [Catch2](https://github.com/catchorg/Catch2)

---

For the full roadmap, see [TODO.md](TODO.md).
