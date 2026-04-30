#!/usr/bin/env python3
"""b33p — cross-platform configure / build / launch helper.

Run from the repo root. Detects the host platform and:
  1. Configures the CMake build directory if it's missing.
  2. Runs an incremental build.
  3. Launches the freshly-built standalone app.

Override the build type with the B33P_BUILD_TYPE env var
(default: Debug for fastest iteration).

Equivalent to the macOS-only `Build and Launch.command` but works
identically on Windows and Linux. Requires `cmake` on PATH; uses
Ninja when available and falls back to the platform default
generator otherwise.
"""

from __future__ import annotations

import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

REPO_ROOT  = Path(__file__).resolve().parent
BUILD_DIR  = REPO_ROOT / "build"
BUILD_TYPE = os.environ.get("B33P_BUILD_TYPE", "Debug")


def find_cmake() -> str:
    cmake = shutil.which("cmake")
    if cmake is None:
        sys.exit(
            "ERROR: cmake not found on PATH.\n"
            "       Install cmake (>=3.22):\n"
            "         macOS  : brew install cmake\n"
            "         Linux  : apt install cmake  (or your distro's package)\n"
            "         Windows: winget install Kitware.CMake")
    return cmake


def pick_generator() -> str | None:
    """Prefer Ninja when installed, else let CMake pick."""
    if shutil.which("ninja"):
        return "Ninja"
    return None   # CMake's platform default (Make, Visual Studio, etc.)


def configure(cmake: str) -> None:
    if (BUILD_DIR / "CMakeCache.txt").exists():
        return
    args = [cmake, "-B", str(BUILD_DIR), f"-DCMAKE_BUILD_TYPE={BUILD_TYPE}"]
    gen = pick_generator()
    if gen is not None:
        args.extend(["-G", gen])
    print(f"--> Configuring (generator: {gen or 'default'})")
    subprocess.run(args, cwd=REPO_ROOT, check=True)


def build(cmake: str) -> None:
    print(f"--> Building ({BUILD_TYPE})")
    subprocess.run(
        [cmake, "--build", str(BUILD_DIR),
         "--config", BUILD_TYPE, "--parallel"],
        cwd=REPO_ROOT, check=True)


def standalone_path() -> Path:
    """Resolve the platform-specific standalone artefact path.

    juce_add_plugin places every format under its own subdirectory
    of B33p_artefacts/<config>/. The standalone format is the one
    we launch interactively.
    """
    base = BUILD_DIR / "B33p_artefacts" / BUILD_TYPE / "Standalone"
    system = platform.system()
    if system == "Darwin":
        return base / "b33p.app"
    if system == "Windows":
        return base / "b33p.exe"
    # Linux / other Unix
    return base / "b33p"


def launch(path: Path) -> None:
    if not path.exists():
        sys.exit(f"ERROR: standalone artefact not found at {path}")
    print(f"--> Launching {path}")
    system = platform.system()
    if system == "Darwin":
        subprocess.run(["open", str(path)], check=False)
    elif system == "Windows":
        os.startfile(str(path))    # type: ignore[attr-defined]
    else:
        # Linux: detach so the script returns even though the app
        # keeps running.
        subprocess.Popen([str(path)], cwd=path.parent,
                         start_new_session=True)


def main() -> int:
    print("=" * 36)
    print(" b33p — Build and Launch")
    print(f" Platform   : {platform.system()} {platform.machine()}")
    print(f" Build type : {BUILD_TYPE}")
    print(f" Repo root  : {REPO_ROOT}")
    print("=" * 36)
    print()

    cmake = find_cmake()
    configure(cmake)
    build(cmake)
    launch(standalone_path())
    return 0


if __name__ == "__main__":
    sys.exit(main())
