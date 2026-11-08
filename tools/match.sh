#!/usr/bin/env bash
# usage: match.sh <new_rev> <base_rev> [tc]
set -euo pipefail
NEW=$(./tools/build_commit.sh "${1:?new}")
BASE=$(./tools/build_commit.sh "${2:?base}")
TC=${3:-10+0.1}
N=$(git rev-parse --short=7 "$1"); B=$(git rev-parse --short=7 "$2")
mkdir -p results

fastchess \
  -engine cmd="$ENGINE1_CMD" name="$ENGINE1_NAME" \
  -engine cmd="$ENGINE2_CMD" name="$ENGINE2_NAME" \
  -each tc="$TC" \
  -rounds 5000 -repeat -concurrency 4 \
  -openings file=opening_books/8moves_v3.pgn format=pgn order=random \
  -sprt elo0=0 elo1=10 alpha=0.05 beta=0.05 \
  -log file="results/${ENGINE1_NAME}_vs_${ENGINE2_NAME}.log"