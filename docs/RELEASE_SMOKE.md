# Release smoke-test runbook

Per-OS verification for a `v*` draft release before publishing. Use this any time `gh release view <tag>` shows `isDraft: true`. The procedure is the same for every pre-1.0 tag; the URL and version numbers change.

> The artefacts you smoke-test must come from the **GitHub Releases page**, not from a local `cmake --build`. The whole point is to verify what users will actually download.

## Inputs

- Tag name (e.g. `v0.2.0`)
- Three artefact archives attached to the draft release:
  - `b33p-<version>-macos-universal.zip`
  - `b33p-<version>-windows-x64.zip`
  - `b33p-<version>-linux-x86_64.tar.gz`
- A Mac, a Windows machine, and a Linux machine (or VMs). At least one DAW per platform that supports the relevant plugin format.

## What "passing" means on every OS

All three of the following on every OS:

1. Standalone app launches without an OS gatekeeper hard-block (a one-time warning is expected — see per-OS bypass below).
2. From inside the standalone: the Audition button produces sound, **File ▸ Browse Presets** loads a factory preset, and Export renders a non-empty WAV file.
3. The VST3 (and AU, macOS only) is scanned and instantiable in at least one host DAW and produces sound from a pattern.

If any of those fails on any OS, **do not publish.** Jump to *If a smoke test fails*.

---

## macOS

Tested on the latest two macOS releases the maintainer has access to. Requires an Apple-Silicon or Intel Mac; the bundle is Universal.

```sh
# 1. Download the archive from the draft release.
#    Replace <version> with the tag without the leading "v".
version=0.2.0
cd ~/Downloads
unzip -q "b33p-${version}-macos-universal.zip"
cd "b33p-${version}-macos-universal"

# 2. Strip the quarantine attribute (unsigned binary) and confirm
#    the app launches.
xattr -dr com.apple.quarantine ./Standalone/b33p.app
open ./Standalone/b33p.app
```

Inside the running standalone, verify:

- [ ] Window opens, title reads `b33p — Untitled`.
- [ ] Click **Audition** in Master. A tone plays.
- [ ] **File ▸ Browse Presets** opens. Load *FM Bell*. Audition. Tone changes audibly.
- [ ] **Export…** → render a 1-variation WAV to Desktop. Open the file in QuickTime or Finder preview. Plays back, non-silent.

Then the plugins:

```sh
# 3. Install both plugin formats into the standard search paths.
mkdir -p ~/Library/Audio/Plug-Ins/VST3 ~/Library/Audio/Plug-Ins/Components
cp -R ./VST3/b33p.vst3       ~/Library/Audio/Plug-Ins/VST3/
cp -R ./AU/b33p.component    ~/Library/Audio/Plug-Ins/Components/

# 4. Validate the AU. A passing `auval -v` is the load-bearing check
#    for AU hosts (Logic, GarageBand). Plugin code "B33p", manufacturer "Zqsf".
auval -v aumu B33p Zqsf
```

`auval` should print `AU VALIDATION SUCCEEDED` near the end. Anything else is a fail.

Open at least one DAW from the list below (whichever you have installed) and verify b33p shows up and plays:

- [ ] Logic Pro — rescan Audio Units in *Logic Pro ▸ Settings ▸ Plug-In Manager*. b33p appears under *ZQ SFX*. Insert on an instrument track. MIDI notes trigger sound.
- [ ] Ableton Live — *Preferences ▸ Plug-Ins ▸ Rescan*. b33p appears. Drop on a MIDI track. Notes trigger sound.
- [ ] Reaper — *Preferences ▸ Plug-ins ▸ VST ▸ Re-scan*. b33p appears under *VST3*. Insert as virtual instrument. Notes trigger sound.

## Windows

Tested on Windows 10 or 11.

```powershell
# 1. Download and extract.
$version = "0.2.0"
cd $env:USERPROFILE\Downloads
Expand-Archive ".\b33p-$version-windows-x64.zip" -DestinationPath ".\b33p-$version-windows-x64"
cd ".\b33p-$version-windows-x64"

# 2. Launch the standalone. First launch triggers SmartScreen.
.\Standalone\b33p.exe
```

When SmartScreen appears: **More info → Run anyway**. This is a one-time approval per file.

Inside the running standalone:

- [ ] Window opens, title reads `b33p — Untitled`.
- [ ] **Audition** in Master plays a tone.
- [ ] **File ▸ Browse Presets** loads a factory preset. Tone changes.
- [ ] **Export…** renders a WAV. Plays back outside b33p.

Then the VST3:

```powershell
# 3. Install the VST3 into the standard per-user search path.
$vst3 = "$env:CommonProgramFiles\VST3"
mkdir -Force $vst3 | Out-Null
Copy-Item -Recurse -Force ".\VST3\b33p.vst3" $vst3
```

Open a DAW you have and verify:

- [ ] Reaper — *Options ▸ Preferences ▸ Plug-ins ▸ VST*. Add `C:\Program Files\Common Files\VST3` if not already in the path. Re-scan. b33p shows up. Insert. Notes trigger sound.
- [ ] Ableton Live — *Preferences ▸ Plug-Ins*. Enable VST3, set the same VST3 path, rescan. b33p shows. Drop on a MIDI track. Notes trigger sound.
- [ ] FL Studio / Cubase / Studio One — any one that scans the standard VST3 path is sufficient.

## Linux

Tested on Ubuntu 22.04 or later. The release archive contains the same Standalone + VST3 layout as the other OSes.

```sh
version=0.2.0
cd ~/Downloads
tar -xzf "b33p-${version}-linux-x86_64.tar.gz"
cd "b33p-${version}-linux-x86_64"

# 1. Make the standalone executable and launch.
chmod +x ./Standalone/b33p
./Standalone/b33p
```

Inside the running standalone:

- [ ] Window opens.
- [ ] **Audition** plays a tone (verify your default ALSA / PulseAudio / PipeWire output is wired — if silent, route via the wrapper's gear icon ▸ Audio Settings, *not* a system mixer change).
- [ ] **File ▸ Browse Presets** loads a factory preset. Tone changes.
- [ ] **Export…** renders a WAV. Plays back via `aplay` or your file manager preview.

Then the VST3:

```sh
# 2. Install into the per-user VST3 path.
mkdir -p ~/.vst3
cp -r ./VST3/b33p.vst3 ~/.vst3/
```

- [ ] Reaper — *Options ▸ Preferences ▸ Plug-ins ▸ VST*. Add `~/.vst3` if not present, rescan. b33p shows. Notes trigger sound.
- [ ] Bitwig Studio (if available) — *Settings ▸ Locations ▸ Plug-In Locations*. Add `~/.vst3`. Rescan. Confirm load + sound.

---

## If a smoke test passes everywhere

Publish the draft. Two-step:

```sh
# Confirm the draft is what you expect.
gh release view <tag>      # e.g. v0.2.0

# Flip draft → published. Leave --prerelease on for pre-1.0 tags.
gh release edit <tag> --draft=false
```

For pre-1.0 tags, the release workflow already set `--prerelease=true`; that flag should stay on so installers / DAW updaters don't promote a 0.x build as stable. Drop `--prerelease=true` only when cutting `v1.0.0`.

After publish, the canonical URL becomes `https://github.com/themightyzq/b33p/releases/tag/<tag>` (the draft's `untagged-<hash>` URL stops resolving). Announcements, README badges, and any external links should point at the canonical URL.

## If a smoke test fails

Two paths, with the choice gated by *whether the draft has been published yet*.

### Pre-publish (draft still draft) — re-tag

Cleanest because nothing public has been promised yet. The old tag never reaches the user.

```sh
# 1. Delete the draft release on GitHub. This also detaches the
#    uploaded archives.
gh release delete <tag> --yes

# 2. Delete the tag locally and on the remote.
git tag -d <tag>
git push origin :refs/tags/<tag>

# 3. Fix the underlying issue on main. Land it as a normal commit
#    (or commits) with a passing CI run.

# 4. Re-tag at the new HEAD. The release workflow rebuilds and
#    re-attaches the archives to a fresh draft.
git tag <tag>
git push origin <tag>
```

Then repeat the smoke tests against the new archives.

### Post-publish (already shipped) — fix-forward as v0.X+1

Once `--draft=false` has run, do not delete the tag — anyone who downloaded already has it, and rewriting public tags is a churn pattern installers handle badly.

```sh
# 1. Fix the issue on main.
# 2. Tag the next patch.
git tag v0.X+1
git push origin v0.X+1
```

The release workflow will produce a fresh draft. Smoke-test it before publish.

---

## Maintenance

This runbook is per-OS and per-tag-name only. Anything that changes the artefact layout, the plugin formats produced, or the install paths should land in this file at the same time the workflow change lands — both files describe the same release surface.
