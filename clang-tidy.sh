#!/usr/bin/env bash
#
# Run clang-tidy over b33p's own sources (Source/*.cpp) the way the CI
# `clang-tidy` job does, but locally — so lint findings are caught before
# pushing instead of via a red-CI round-trip.
#
# Why this script exists: on macOS the build is Universal (two -arch flags),
# so the generated compile_commands.json has two compiler jobs per file and
# clang-tidy bails with "expected exactly one compiler job". We patch a
# single-arch copy of compile_commands.json for clang-tidy to consume. On
# Linux / single-arch builds the patch is a harmless no-op.
#
# Caveat: your local clang-tidy version may differ from CI's, so this is a
# strong pre-push smoke check, not a bit-for-bit replica. The .clang-tidy
# config (auto-discovered by walking up from each file) is the source of
# truth for the check set + WarningsAsErrors.
#
# Usage:  ./clang-tidy.sh
#   CLANG_TIDY=/path/to/clang-tidy ./clang-tidy.sh   # override the binary
set -uo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD="$ROOT/build"

# CI pins clang-tidy to this major version (see .github/workflows/build.yml).
# Matching it locally is what makes this a faithful pre-push check — a newer
# clang-tidy reports a superset of findings the gate doesn't enforce.
CI_MAJOR=18

# Locate a clang-tidy, preferring the CI-pinned major for parity:
#   $CLANG_TIDY override, clang-tidy-18 on PATH, Homebrew llvm@18, then any.
CT="${CLANG_TIDY:-}"
if [ -z "$CT" ]; then
    if command -v "clang-tidy-$CI_MAJOR" >/dev/null 2>&1; then
        CT="clang-tidy-$CI_MAJOR"
    elif [ -x "/opt/homebrew/opt/llvm@$CI_MAJOR/bin/clang-tidy" ]; then
        CT="/opt/homebrew/opt/llvm@$CI_MAJOR/bin/clang-tidy"
    elif command -v clang-tidy >/dev/null 2>&1; then
        CT="clang-tidy"
    elif [ -x /opt/homebrew/opt/llvm/bin/clang-tidy ]; then
        CT="/opt/homebrew/opt/llvm/bin/clang-tidy"
    else
        echo "ERROR: clang-tidy not found. For CI parity: brew install llvm@$CI_MAJOR" >&2
        echo "       (or set CLANG_TIDY=/path/to/clang-tidy)." >&2
        exit 127
    fi
fi

# Warn loudly if the local major differs from CI's — findings may not match.
CT_MAJOR="$("$CT" --version | sed -nE 's/.*version ([0-9]+).*/\1/p' | head -1)"
if [ "$CT_MAJOR" != "$CI_MAJOR" ]; then
    echo "WARNING: using clang-tidy $CT_MAJOR but CI pins $CI_MAJOR — findings may differ." >&2
    echo "         For an exact match: brew install llvm@$CI_MAJOR" >&2
fi

# Ensure compile_commands.json exists (configure if needed; the Universal
# arch is fine — we strip it below).
if [ ! -f "$BUILD/compile_commands.json" ]; then
    echo "--> No compile_commands.json; configuring $BUILD ..."
    cmake -B "$BUILD" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON >/dev/null
fi

# Patch a single-arch copy: keep the first "-arch <x>", drop the rest, leaving
# the rest of each command byte-identical (no re-tokenising, so quoted defines
# with spaces survive).
TIDYDIR="$BUILD/.clang-tidy-cc"
mkdir -p "$TIDYDIR"
python3 - "$BUILD/compile_commands.json" "$TIDYDIR/compile_commands.json" <<'PY'
import json, re, sys
src, dst = sys.argv[1], sys.argv[2]
db = json.load(open(src))
for e in db:
    c = e.get("command", "")
    arches = re.findall(r'-arch \S+', c)
    for extra in arches[1:]:           # keep the first, remove any others
        c = c.replace(' ' + extra, '', 1)
    e["command"] = c
json.dump(db, open(dst, "w"))
PY

echo "--> clang-tidy: $("$CT" --version | sed -n 's/.*version /v/p' | head -1)"
echo "--> Analysing $(find "$ROOT/Source" -name '*.cpp' | wc -l | tr -d ' ') translation units ..."

LOG="$BUILD/tidy.log"
find "$ROOT/Source" -name '*.cpp' -print0 \
    | xargs -0 -P "$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)" -I{} \
        "$CT" -p "$TIDYDIR" --quiet "{}" \
    > "$LOG" 2>&1
status=$?

echo "---- summary ----"
findings=$(grep -cE 'warning:|error:' "$LOG" || true)
echo "clang-tidy findings: ${findings:-0}"
echo "---- by check ----"
grep -oE '\[[a-z0-9.-]+\]$' "$LOG" | sort | uniq -c | sort -rn || true
if [ "${findings:-0}" != "0" ]; then
    echo "---- findings in Source/ ----"
    grep -E 'Source/.*:[0-9]+:[0-9]+: (warning|error):' "$LOG" | sed "s#$ROOT/##" || true
fi
exit "$status"
