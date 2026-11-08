#!/usr/bin/env bash
# usage: build_commit.sh <rev>   -> prints path to the binary
set -euo pipefail
REV=${1:?rev}
ROOT=$(git rev-parse --show-toplevel)
HASH=$(git rev-parse --short=10 "$REV")
OUT="$ROOT/engines/$HASH"
BIN="$OUT/engine"

if [[ -x "$BIN" ]]; then echo "$BIN"; exit 0; fi   # cached


WT="$ROOT/engines/.wt-$HASH"

cleanup() {
    git worktree remove --force "$WT" >/dev/null 2>&1 || true
    git worktree prune >/dev/null 2>&1 || true
}
trap cleanup EXIT

git worktree prune                      # drop stale registrations
rm -rf "$WT"                            # drop leftover directory, if any
mkdir -p "$OUT"
git worktree add --detach "$WT" "$HASH" >&2
(
  cd "$WT"
  mkdir -p build bin
  make type=release
  cp bin/filipbot "$BIN"
)
git worktree remove --force "$WT" >&2
echo "$BIN"
