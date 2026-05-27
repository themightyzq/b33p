# b33p — End-User Review (2026-05-27)

Reviewed as an experienced sound-design / game-audio user with high standards, encountering b33p fresh. Method: read README + `docs/USAGE.md` as a user (skim-first), checked the GitHub release/download state, and ran the macOS standalone and clicked through the editor. Journeys that need a real mouse on JUCE controls couldn't be fully automated, so a few items are reasoned from the live UI + docs rather than click-tested; those are marked.

**Overall:** the app itself is polished, fast, and genuinely capable — a real synth, not a toy. The problems are almost all at the **edges**: how you get it, and how a first-timer gets from launch to "one tiny sound." The core, once you're in, is good.

---

## Broken — told it works, but doesn't for a real user

### B1. The Releases page has nothing to download
- **Symptom:** README's primary install step says "go to the Releases page and download the archive for your OS." A public visitor sees an **empty Releases page** — the only release (`v0.2.0`) is an unpublished **Draft**, invisible to non-collaborators.
- **Where:** `README.md` → Installation → [Releases](https://github.com/themightyzq/b33p/releases).
- **Severity:** blocker.
- **Fix:** publish the staged `v0.2.0` release (it's built and attached, just draft), or change the README so the lead install path matches what's actually downloadable.

### B2. "Binaries on each Actions run" can't be downloaded without a GitHub login
- **Symptom:** README Status + Installation point users to GitHub Actions artifacts for binaries. Workflow artifacts are **not downloadable anonymously** — a logged-out user gets no download button.
- **Where:** `README.md` Status + "Bleeding-edge `main` builds."
- **Severity:** major.
- **Fix:** state the login requirement, or attach binaries to a published (pre)release as the canonical channel.

### B3. USAGE points at a "Lane" menu that no longer exists
- **Symptom:** "use **Lane → Generate Random Pattern in Lane N** from the menu bar" — there is no Lane menu; the menu bar is File · Edit · Help and the action lives under `Edit ▸ Lane`. A user hunting the menu bar won't find it.
- **Where:** `docs/USAGE.md:193`.
- **Severity:** minor.
- **Fix:** change to `Edit ▸ Lane ▸ Generate Random Pattern…` (USAGE already uses `Edit ▸ Lane` correctly elsewhere — line 213 — so it's internally inconsistent).

---

## Missing — reasonably expected, not there

### M1. No published / versioned binary at all
- **Symptom:** even `v0.1.0` was only ever a git tag, never a published GitHub Release. There is no stable, versioned download a user can grab.
- **Where:** distribution.
- **Severity:** blocker (adoption).
- **Fix:** publish releases; the build pipeline already produces the archives.

### M2. No way to export a single sound — export is pattern-only
- **Symptom:** the headline use case is "make a lot of good tiny sounds, quickly," but **Export renders the pattern**. To get one beep you must first draw an event on the grid; an empty pattern exports silence. Auditioning a voice doesn't export it.
- **Where:** Export… flow vs. the Audition button.
- **Severity:** major. The closest comparables (sfxr / Bfxr / jsfxr / ChipTone) export the *current* sound in one click — that's the expectation for this category.
- **Fix:** add "Export current voice as one-shot" (render one audition note + tail), or auto-render a single hit when Export runs on an empty pattern.

### M3. No in-app onboarding
- **Symptom:** first launch is a dense, ~7-section modular synth + a pattern grid. The only guidance is a small empty-pattern hint. A "fast iteration" tool opens looking like a full DAW plugin.
- **Where:** first launch.
- **Severity:** minor.
- **Fix:** a one-time welcome panel or a "New to b33p? Quick start" item under Help that mirrors the README quickstart.

### M4. No documented uninstall / data-location story
- **Symptom:** docs never say where presets and settings live for cleanup (presets in `~/Library/Application Support/b33p/Presets`, plus a `.settings` file). A tidy user can't fully remove it.
- **Where:** docs.
- **Severity:** polish.
- **Fix:** add an "Uninstall / where your data lives" note to USAGE.

---

## Confusing — works, but causes friction or self-doubt

### C1. "Each lane is its own voice" isn't obvious in-app
- **Symptom:** the whole mental model is "4 pattern lanes, each with an independent voice; click a lane to retarget the editor." In-app, the only cue is the small "(Lane 1)" suffix on the Oscillator title. A newcomer may not connect the pattern lanes to the voice editor at all.
- **Where:** voice editor ↔ pattern grid relationship.
- **Severity:** major (it's the central concept).
- **Fix:** a first-run one-liner or persistent micro-hint: "Each of the 4 lanes has its own voice — click a lane to edit it."

### C2. The most useful actions are right-click-only and invisible
- **Symptom:** Generate random pattern, Clear lane, Rename lane, Park playhead (click ruler), Edit per-event overrides — all hidden behind right-click with nothing on-screen advertising them. Documented in README/USAGE, but a user who doesn't read docs never finds them.
- **Where:** pattern grid.
- **Severity:** minor.
- **Fix:** surface the top two (generate / rename) in the empty-state hint or a small "⋯" affordance.

### C3. Pressing Play on an empty pattern does nothing, silently
- **Symptom:** two ways to hear sound — Audition (one voice) and Space/Play (the pattern). A first-timer who hits Play before drawing a clip hears silence and no explanation. *(Reasoned from the UI; not click-tested.)*
- **Where:** transport / pattern grid.
- **Severity:** minor.
- **Fix:** when Play starts on an empty pattern, flash a hint ("Pattern is empty — draw a clip, or press Shift+Space to audition").

### C4. The modulation matrix is dense for a newcomer
- **Symptom:** four rows of Source / Destination / bipolar Amount with one italic hint line. Powerful, but a wall on first contact. (The new per-row activity glow helps once something is routed.)
- **Where:** Modulation section.
- **Severity:** minor.
- **Fix:** leave as-is for now; the activity indicator already improved this materially.

---

## Recommended — would meaningfully raise quality

- **R1.** Publish a release and make the README's lead install path one that yields a binary today (ties off B1/B2/M1). Single highest-leverage change for adoption.
- **R2.** One-shot export for the single-sound use case (M2) — it's what the tagline promises.
- **R3.** Empty-pattern Play feedback (C3) and a one-line lane/voice explainer (C1) — cheap, high-impact onboarding wins.

## Nice-to-have — lower-priority polish

- **N1.** Preset browser categories / tags / search — it's a flat alphabetical list; category synths (Vital, Serum, Pigments) set the expectation. (Already tracked as REVIEW P19.)
- **N2.** Screen-reader labels on custom controls — automation couldn't find the preset `< >` buttons by accessible name, suggesting some JUCE custom controls don't expose names to assistive tech. *(Observed via UI automation, not a real screen reader — worth a pass.)*
- **N3.** A short troubleshooting/FAQ (install gate per OS exists in README; add "no sound?", "plugin doesn't show in my DAW", "where are my presets?").

---

## Top User Frustrations (ordered by likelihood of making someone give up)

1. **You can't actually download it.** The README says "download from Releases," the Releases page is empty, and the Actions fallback needs a login — so the real install is *build a C++ toolchain from source* (13 apt packages on Linux, Xcode on macOS, VS2022 on Windows). Most evaluators bounce right here. (B1, B2, M1)
2. **"Where do I make one quick sound?"** The headline promise is tiny sounds fast, but you must learn the pattern grid and place an event before you can export anything — there's no one-shot export. (M2)
3. **Dense first screen, no onboarding.** A "fast iteration" tool opens as a full modular synth with zero "start here." (M3)
4. **The lane↔voice model is non-obvious.** Users may not realize each pattern lane is a separate voice the editor retargets to. (C1)
5. **Key gestures are hidden in right-click menus** with nothing on-screen to hint they exist. (C2)

*Note: B1/M1 are a deliberate hold — the v0.2.0 release is staged but intentionally unpublished pending smoke tests — so the "fix" is largely "publish," not "build something." It's still the #1 thing a fresh user hits.*
