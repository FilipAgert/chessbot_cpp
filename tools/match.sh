#!/usr/bin/env bash
# usage: match.sh <new_rev> <base_rev> [tc]
set -euo pipefail
NEW=$(./tools/build_commit.sh "${1:?new}")
BASE=$(./tools/build_commit.sh "${2:?base}")
TC=${3:-10+0.1}
N=$(git rev-parse --short=7 "$1"); B=$(git rev-parse --short=7 "$2")
mkdir -p results

fastchess \
  -engine cmd="$NEW"  name="$N" \
  -engine cmd="$BASE" name="$B" \
  -each proto=uci tc="$TC" option.Hash=64 \
  -openings file=books/book.epd format=epd order=random \
  -games 2 -rounds 5000 -repeat \
  -concurrency 4 \
  -sprt elo0=0 elo1=5 alpha=0.05 beta=0.05 \
  -pgnout file="results/${N}_vs_${B}.pgn" \
  | tee "results/${N}_vs_${B}.log"
