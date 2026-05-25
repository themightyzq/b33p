#!/usr/bin/env bash
# B33p — Build and Launch
#
# Double-click this file in Finder to configure (first time), build (incremental),
# and launch the B33p standalone app. macOS-only helper.
#
# Override the build type with e.g. `B33P_BUILD_TYPE=Release` if needed.

set -euo pipefail

# Jump into this script's own directory so relative paths work regardless of how
# Finder launched us.
cd "$(dirname "${BASH_SOURCE[0]}")"

BUILD_DIR="build"
BUILD_TYPE="${B33P_BUILD_TYPE:-Debug}"
APP_BUNDLE="${BUILD_DIR}/B33p_artefacts/${BUILD_TYPE}/Standalone/b33p.app"

echo "=============================="
echo " b33p — Build and Launch"
echo " Build type : ${BUILD_TYPE}"
echo " Working dir: $(pwd)"
echo "=============================="
echo

if ! command -v cmake >/dev/null 2>&1; then
    echo "ERROR: cmake not found on PATH." >&2
    echo "       Install with: brew install cmake" >&2
    exit 1
fi

# Reconfigure when the cache is missing OR when its cached build type
# differs from what we're being asked for. Single-config generators like
# Unix Makefiles bake the build type at configure time and silently ignore
# `cmake --build --config <X>`, so without this check a switch from
# Release to Debug (or vice versa) just rebuilds the wrong type and the
# artefact-path lookup below fails.
#
# Generators are immutable for a given build dir — CMake errors if `-G`
# is passed with a different value than the cached one. So we only pick
# a generator on a *fresh* configure; on reconfigure we omit `-G` and
# let CMake reuse the cached generator.
CACHED_TYPE=""
CACHED_GENERATOR=""
if [ -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    CACHED_TYPE=$(grep '^CMAKE_BUILD_TYPE:STRING='        "${BUILD_DIR}/CMakeCache.txt" | cut -d= -f2)
    CACHED_GENERATOR=$(grep '^CMAKE_GENERATOR:INTERNAL='  "${BUILD_DIR}/CMakeCache.txt" | cut -d= -f2)
fi

if [ "${CACHED_TYPE}" != "${BUILD_TYPE}" ]; then
    if [ -z "${CACHED_TYPE}" ]; then
        if command -v ninja >/dev/null 2>&1; then
            GENERATOR="Ninja"
        else
            GENERATOR="Unix Makefiles"
        fi
        echo "--> Configuring with ${GENERATOR} (${BUILD_TYPE})"
        cmake -B "${BUILD_DIR}" -G "${GENERATOR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
    else
        echo "--> Reconfiguring with ${CACHED_GENERATOR} (${CACHED_TYPE} -> ${BUILD_TYPE})"
        cmake -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
    fi
    echo
fi

echo "--> Building"
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" --parallel
echo

if [ ! -d "${APP_BUNDLE}" ]; then
    echo "ERROR: expected app bundle not found at ${APP_BUNDLE}" >&2
    exit 1
fi

echo "--> Launching ${APP_BUNDLE}"
open "${APP_BUNDLE}"
