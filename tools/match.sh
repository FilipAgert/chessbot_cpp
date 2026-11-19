#!/usr/bin/env bash
# usage: match.sh <new_rev> <base_rev> [num_threads] [tc]
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"

ENGINE1_CMD=$(./tools/build_commit.sh "${1:?new}")
ENGINE2_CMD=$(./tools/build_commit.sh "${2:?base}")
ENGINE1_NAME=$(git rev-parse --short=7 "$1")
ENGINE2_NAME=$(git rev-parse --short=7 "$2")
TC=${4:-3+0.1}
mkdir -p results
NT=${3:-4}
fastchess \
  -engine cmd="$ENGINE1_CMD" name="$1" \
  -engine cmd="$ENGINE2_CMD" name="$2" \
  -each tc="$TC" \
  -rounds 5000 -repeat -concurrency "$NT" \
  -recover \
  -openings file=opening_books/8moves_v3.pgn format=pgn order=random \
  -sprt elo0=0 elo1=15 alpha=0.05 beta=0.05 \
  -log file="results/${ENGINE1_NAME}_vs_${ENGINE2_NAME}.log"