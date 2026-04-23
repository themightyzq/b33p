# CLAUDE.md

Operating rules for Claude Code sessions in this repository. **Read this at the start of every session.** Every rule here exists to keep the project from bloating past its purpose.

## Project identity

- **Name:** B33p
- **Display name:** B33p (window title, About box, README)
- **C++ namespace:** `B33p`
- **CMake project:** `B33p`
- **Bundle identifier:** `com.themightyzq.b33p`
- **Saved project extension:** `.beep`
- **Author:** ZQ SFX — GitHub [themightyzq](https://github.com/themightyzq)
- **License:** GPL-3.0-or-later

## Scope discipline

1. **Never build out-of-phase features.** Before writing any code, open `TODO.md` and confirm the task belongs to the **current phase** (marked at the top of the file).
2. **If the user asks for something out of phase,** push back. Acknowledge the idea, note where it belongs in the roadmap, and offer to add it to `TODO.md` under the appropriate post-MVP section. Do not implement it.
3. **Simplicity wins every tie.** When choosing between a smaller and a more complete version of a feature, ship the smaller one.
4. **No speculative abstractions.** Build the concrete thing first. Extract abstractions only on the third repetition, not the first.
5. **Flag feature creep.** If a task starts growing beyond its stated MVP definition, stop and tell the user before continuing.

## Architecture

All code lives under `Source/`, organized by concern. One class per file. Everything in the `B33p` namespace.

```
Source/
├── Core/      # shared utilities, constants, parameter IDs, ranges
├── DSP/       # oscillator, envelopes, filter, effects — pure audio, no UI
├── Pattern/   # pattern data model, lanes, events, sequencer, playhead
├── State/     # APVTS wiring, ValueTree, project save/load, versioning
└── UI/        # components, editors, drawable pitch envelope, timeline
```

Rules:
- **Headers stay minimal.** Implementation in `.cpp`. No large inline blobs in headers.
- **DSP knows nothing about UI.** UI observes state; DSP reads parameters. They never reference each other.
- **State is the single source of truth.** Parameters flow through `AudioProcessorValueTreeState`. Project-level state lives in a `ValueTree`. UI and DSP both read from these.

## JUCE rules

- Use `AudioProcessorValueTreeState` for **every** parameter from day one. No ad-hoc `std::atomic<float>` members.
- Use JUCE DSP module classes (`juce::dsp::IIR`, `juce::dsp::Oscillator`, `juce::dsp::WaveShaper`, etc.) where they exist. Do not reimplement filters, oscillators, or waveshapers from scratch.
- Use `UndoManager` for **every** user-initiated state change, including dice rolls (one transaction per click).
- Use `ValueTree` for project serialization. Version the tree from the first save. Never break older `.beep` files without an explicit migration.
- Prefer JUCE containers (`juce::Array`, `juce::String`) in JUCE-facing code; prefer `std::` containers in pure DSP/Core code.

## Code style

- **C++17.** No C++20 features unless the user approves.
- **RAII.** No raw `new`/`delete`. Owning pointers are `std::unique_ptr`. Non-owning references use `&` or a raw pointer with a comment naming the owner.
- **JUCE naming conventions:** `PascalCase` types, `camelCase` methods and members. No `m_` or `_` prefixes.
- **Small functions.** If a function doesn't fit on a screen, split it.
- **Explicit types** for non-obvious cases. Use `auto` only when the type is obvious from the RHS or genuinely verbose (iterators, lambdas).
- **Comments explain *why*, not *what*.** Name things well enough that the *what* is obvious. Good comments document non-obvious constraints, workarounds, and invariants.

## Build

- **CMake only.** No Projucer.
- **JUCE via CPM.** One dependency system, consistent across platforms. Do not mix `FetchContent` and CPM — CPM is the choice.
- **Must build clean on macOS, Windows, and Linux.** No `#ifdef __APPLE__` in MVP unless a specific task explicitly requires it.
- **Never commit build artifacts.** `build/`, `Builds/`, `.vs/`, `cmake-build-*` are gitignored.

## Testing

- **DSP classes require unit tests.** Use **Catch2** (chosen over JUCE's `UnitTest` for better CTest/CI integration).
- **UI does not require tests in MVP.** Ship manual-test notes in the PR description if behavior is non-obvious.
- **Every bug fix adds a regression test.** No exceptions.
- Tests live in `Tests/` mirroring the `Source/` layout.

## Workflow

1. Read `TODO.md`. Confirm the task is in the active phase.
2. Skim the **"Chores / tech debt"** section near the bottom of `TODO.md`. If any item has a time-sensitive deadline approaching (next two weeks) or is newly relevant to the work at hand, flag it to the user before starting. When chores are resolved, check them off rather than deleting them — same rule as phase tasks.
3. If anything about scope is unclear, ask before writing code.
4. Implement. Run tests. Build on at least the host OS.
5. Check the task off in `TODO.md` (do **not** delete it — history matters).
6. Commit with Conventional Commits: `feat:`, `fix:`, `refactor:`, `docs:`, `test:`, `build:`, `chore:`.

## When to ask vs. proceed

**Ask first:**
- Changing architecture or adding a top-level folder
- Adding or removing a dependency
- Reinterpreting the scope of a TODO item
- Breaking the `.beep` file format (always — with a migration plan)
- Touching anything outside `Source/`, `Tests/`, `CMakeLists.txt`, or the three root docs

**Proceed without asking:**
- Implementing a clearly scoped TODO task inside its described boundaries
- Adding unit tests, even when not explicitly requested
- Refactoring within a single file for clarity without changing behavior
- Fixing a bug you find along the way (note it in the commit message)
