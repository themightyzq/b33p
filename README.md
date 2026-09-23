# b33p

b33p is a synth for short sound-effect beeps, blips, alarms, and droid chatter. Design a
voice, lay it out on a four-lane pattern sequencer, hit Randomize until something
surprises you, then export to WAV, AIFF, FLAC, or OGG. Load it as Standalone, VST3, or
AU and play it from a MIDI keyboard. Built with JUCE for macOS, Windows, and Linux.

## Install

Download the latest release from the
[Releases page](https://github.com/themightyzq/b33p/releases/tag/v0.2.0) (v0.2.0,
pre-release, 2026-05-27):

- macOS: `b33p-0.2.0-macos-universal.zip`
- Windows: `b33p-0.2.0-windows-x64.zip`
- Linux: `b33p-0.2.0-linux-x86_64.tar.gz`

Each archive bundles the Standalone app, VST3, and (macOS) AU plugins.

b33p is unsigned, so each OS gates the first launch:

- macOS: right-click the app and choose Open (a plain double-click shows "cannot be
  opened because the developer cannot be verified" with no Open button). Or strip the
  quarantine attribute: `xattr -dr com.apple.quarantine /path/to/b33p.app`
- Windows: SmartScreen shows "Windows protected your PC". Click More info, then Run
  anyway.
- Linux: `chmod +x b33p`, then run it.

After the first successful launch on each OS, b33p runs normally. Or build from source
(below).

## Use

Make a beep:

1. Launch b33p (Standalone, VST3, or AU, same UI).
2. Press Audition (or `Shift+Space`) in the Master strip to hear the default sine beep.
3. Drag any knob to taste: pitch, filter cutoff, drive, mod-FX mix.
4. Hit Randomize Voice to roll the dice on the current lane's voice settings until
   something surprises you. Press Audition again to hear it.

From there, draw clips in the pattern grid to sequence the beep, or play it live from a
MIDI keyboard.

Shortcuts you'll want early on:

| Shortcut | Action |
| --- | --- |
| `Space` | Play/stop the pattern |
| `Shift+Space` | Audition the selected lane's voice |
| `Cmd+S` | Save |
| `Cmd+Z` / `Cmd+Shift+Z` | Undo / redo |

On Windows and Linux, use `Ctrl` in place of `Cmd`. The full shortcut list is under
Help > Keyboard Shortcuts in the app.

For the complete reference (voice model, modulation, presets, MIDI), see
[docs/USAGE.md](docs/USAGE.md). For common questions ("I don't hear anything",
"plugin not in my DAW", "where are my presets"), see [docs/FAQ.md](docs/FAQ.md).

## Build from source

Requirements: CMake 3.22 or newer, a C++17 compiler, Git.

macOS:

```sh
git clone https://github.com/themightyzq/b33p.git
cd b33p
cmake -B build -G Xcode
cmake --build build --config Release
```

Output: `build/B33p_artefacts/Release/Standalone/b33p.app`,
`build/B33p_artefacts/Release/VST3/b33p.vst3`,
`build/B33p_artefacts/Release/AU/b33p.component`.

Windows:

```powershell
git clone https://github.com/themightyzq/b33p.git
cd b33p
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

Output: `build\B33p_artefacts\Release\Standalone\b33p.exe`,
`build\B33p_artefacts\Release\VST3\b33p.vst3`. AU is macOS-only and is skipped
automatically.

Linux:

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

Output: `build/B33p_artefacts/Release/Standalone/b33p`,
`build/B33p_artefacts/Release/VST3/b33p.vst3`.

## Licence

GPL-3.0-or-later. See `LICENSE`. Built with JUCE.

ZQ SFX, https://www.zq-sfx.com, connect@zq-sfx.com.
