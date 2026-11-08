#!/usr/bin/env bash
# usage: match.sh <new_rev> <base_rev> [tc]
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"

ENGINE1_CMD=$(./tools/build_commit.sh "${1:?new}")
ENGINE2_CMD=$(./tools/build_commit.sh "${2:?base}")
ENGINE1_NAME=$(git rev-parse --short=7 "$1")
ENGINE2_NAME=$(git rev-parse --short=7 "$2")
TC=${3:-10+0.1}
mkdir -p results

fastchess \
  -engine cmd="$ENGINE1_CMD" name="$1" \
  -engine cmd="$ENGINE2_CMD" name="$2" \
  -each tc="$TC" \
  -rounds 5000 -repeat -concurrency 4 \
  -openings file=opening_books/8moves_v3.pgn format=pgn order=random \
  -sprt elo0=0 elo1=10 alpha=0.05 beta=0.05 \
  -log file="results/${ENGINE1_NAME}_vs_${ENGINE2_NAME}.log"