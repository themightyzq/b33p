# b33p — FAQ

Short answers to questions a real user is most likely to hit. The long-form guide lives in [`USAGE.md`](USAGE.md).

## Install + first launch

### "I downloaded b33p and macOS / Windows is blocking the launch."

Expected — b33p is unsigned today. Once-per-install bypass:

- **macOS**: right-click the app → **Open** (the regular double-click path shows a "cannot be opened because the developer cannot be verified" dialog with no Open button). Or, in Terminal: `xattr -dr com.apple.quarantine /path/to/b33p.app`
- **Windows**: SmartScreen shows "Windows protected your PC." Click **More info → Run anyway**.
- **Linux**: `chmod +x b33p && ./b33p`.

After the first successful launch on each OS, b33p runs normally without the prompt.

### "I don't hear anything when I press Audition."

Walk these in order:

1. The Master strip has a **Gain** slider. Make sure it's not at the bottom.
2. The Master strip has an output **meter** to the right of the slider. Press Audition — does the meter move? If yes, the synth is producing sound and the problem is downstream (output device).
3. **macOS**: System Settings → Sound → Output — pick the right device. Then restart b33p so it picks up the new device.
4. **Standalone**: Options → Audio/MIDI Settings → confirm the output device + sample rate match your interface.
5. **Plugin in a DAW**: the host probably routed b33p's output to a track that's muted or routed nowhere. Check the routing.

### "I built it from source but the editor doesn't open."

Try the launcher (it builds Debug by default and is faster to iterate):

```sh
python3 build_and_launch.py
# or, on macOS:
"./Build and Launch.command"
```

If the editor still won't open, the standalone may have crashed silently. Run it from a terminal so you can see any output:

```sh
./build/B33p_artefacts/Debug/Standalone/b33p.app/Contents/MacOS/b33p
```

## Plugin in a DAW

### "b33p doesn't show up in my DAW's plugin list."

After install, your DAW typically rescans plugins on next launch — some DAWs (Logic, Live) need you to trigger the rescan manually. Common scan paths:

- **VST3** (every DAW): `~/Library/Audio/Plug-Ins/VST3/` (macOS), `~/.vst3/` (Linux), `C:\Program Files\Common Files\VST3\` (Windows).
- **AU** (Logic, GarageBand, Cubase on macOS): `~/Library/Audio/Plug-Ins/Components/`.

Copy or move b33p's plugin bundle into the relevant directory, then in your DAW: Preferences → Plug-ins → **Rescan**.

If b33p is still missing after rescan, your DAW may have blocklisted it after a previous failed validation. Logic in particular caches AU validation results; you can force a re-validation by deleting `~/Library/Caches/com.apple.audiounits.cache` (macOS) and restarting Logic.

### "The plugin loads but doesn't make sound when I play MIDI notes."

In a DAW, b33p reads MIDI from the track it's loaded on. Make sure:

1. The MIDI track is record-armed (so it accepts live MIDI), or your MIDI clip is actually playing.
2. The MIDI is being routed to b33p — check the track's instrument routing.
3. The DAW's transport is actually rolling (not just the MIDI window in solo).

MIDI input drives the **selected lane**'s voice — switching lanes in b33p's editor changes which voice the MIDI plays. The pattern timeline continues to run alongside MIDI input, so you can layer host MIDI on top of an internal pattern.

### "What does the Follow toggle do?"

`Follow` enables DAW-transport sync. When on:

- Host play / stop drives b33p's `playing` state.
- The pattern playhead snaps to `host_time mod pattern_length` at the top of every audio block, so b33p's pattern stays in alignment with the host's bar lines.

When off (the default), b33p's pattern playhead runs independently of host transport — useful when you want a beep pattern at a different tempo from the host session.

`Follow` is inert in standalone mode (no host = no playhead). The toggle state is saved into the `.beep` so a project saved standalone keeps its choice when reopened in a DAW.

## Presets

### "Where are my presets stored?"

Per-user preset directory:

- **macOS**: `~/Library/Application Support/b33p/Presets/`
- **Windows**: `%APPDATA%\b33p\Presets\`
- **Linux**: `~/.config/b33p/Presets/` (or `$XDG_CONFIG_HOME/b33p/Presets/` if set)

Each preset is a single `.beep` file. You can copy them between machines, email them, drop them into git.

### "I saved over a factory preset by accident. Can I recover?"

Yes. **File ▸ Restore Factory Presets...** rewrites the four shipped presets (FM Bell, Resonant Stab, Delay Pad, Ring Mod Robot) to their originals. Your own presets are untouched.

## Uninstall

### "How do I remove b33p completely?"

1. Delete the app bundle / executable.
2. Delete the plugin bundles you copied into your VST3 / AU directories (see "b33p doesn't show up in my DAW" above for paths).
3. Delete the per-user state directory:
   - **macOS**: `~/Library/Application Support/b33p/` (presets + preferences)
   - **Windows**: `%APPDATA%\b33p\`
   - **Linux**: `~/.config/b33p/` (or `$XDG_CONFIG_HOME/b33p/`)
4. Delete `~/Library/Preferences/com.themightyzq.b33p.plist` on macOS (window geometry + recent-files list).

Nothing else is written outside your home directory.

## Other

### "I want to render JUST one beep, not a pattern."

`Export` currently renders the pattern, not the current voice in isolation. Two-second workaround:

1. Double-click on an empty spot in **Lane 1** of the pattern grid to drop a default-size clip.
2. Open `Export...` and render. You'll get a WAV of one audition-length hit through the current voice.
3. `Cmd+Z` reverts the dropped clip when you're done.

A "Export Current Voice..." menu that skips the clip-drop step is roadmapped — see [`TODO.md`](../TODO.md).

### "Why doesn't `Cmd+Z` undo a Randomize?"

It does — but only once per Randomize *button-press*. The four random rolls inside a single Randomize button-press are grouped into one undoable transaction. So `Cmd+Z` reverts the whole roll back to the pre-randomize patch.

### "Bypass on the plugin freezes my pattern playback."

Correct behaviour. Plugin-bypass on an instrument plugin means "stop generating audio" — the pattern playhead pauses and any events that would have fired during the bypass window are skipped (they don't queue up to fire on un-bypass). On un-bypass the output fades back in over ~10 ms so a long reverb / delay tail that was sounding at engage doesn't resume with a click.

If you want the pattern to keep advancing while b33p is silent, mute the plugin's output bus in your host instead of using plugin-bypass.

### "The download is labeled 'Pre-release.' Is it safe to use?"

The pre-release marker means "not yet 1.0," not "unstable." b33p hits its CI gates (build + tests + clang-tidy v18 + pluginval / auval informational) on every push to `main` before a release is cut. Most tagged versions are intended for production sound-design use; the pre-release label will be promoted to full release once the project hits 1.0.

### "Where do I report a bug?"

Open an issue on [github.com/themightyzq/b33p](https://github.com/themightyzq/b33p). Bug-report and feature-request templates are in `.github/ISSUE_TEMPLATE/` — they walk you through what's most useful to capture (OS, plugin format, host DAW if applicable, steps to reproduce, the `.beep` file if state-dependent).
