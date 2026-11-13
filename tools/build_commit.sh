#!/usr/bin/env bash
# usage: build_commit.sh <rev>   -> prints path to the binary
set -euo pipefail
REV=${1:?rev}
ROOT=$(git rev-parse --show-toplevel)

# 1. Get the full hash for reliable git operations, and a short hash for folder naming
FULL_HASH=$(git rev-parse "$REV")
HASH="${FULL_HASH:0:10}"

OUT="$ROOT/engines/$HASH"
BIN="$OUT/engine"

if [[ -x "$BIN" ]]; then 
    echo "$BIN"
    exit 0 
fi   # cached

WT="${TMPDIR:-/tmp}/engine-wt-$HASH"

cleanup() {
    git worktree remove --force "$WT" >/dev/null 2>&1 || true
    git worktree prune >/dev/null 2>&1 || true
}
trap cleanup EXIT

# 2. Safely clean up any leftover directory or stale registrations first
if [[ -d "$WT" ]]; then
    git worktree remove --force "$WT" >/dev/null 2>&1 || rm -rf "$WT"
fi
git worktree prune >&2

mkdir -p "$OUT"

# 3. Use the FULL_HASH here so git worktree add never fails on the first try
git worktree add --detach "$WT" "$FULL_HASH" >&2

(
  cd "$WT"
  mkdir -p build bin

  # Check if this revision's Makefile defines a 'pgo' target, then attempt PGO build
  if grep -qE '^pgo:' Makefile && make -j1 pgo >&2; then
      echo "Built $HASH with PGO." >&2
  else
      echo "PGO unavailable or failed for $HASH, falling back to standard release build..." >&2
      rm -rf build bin
      mkdir -p build bin
      make -j1 type=release all >&2
  fi

  cp bin/filipbot "$BIN"
)

if [[ -x "$BIN" ]]; then
    echo "$BIN"
    exit 0
fi

exit 1