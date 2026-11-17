#!/usr/bin/env bash
# Profile the engine with perf or callgrind by feeding it UCI commands.
set -eu

usage() {
  cat <<EOF
Usage: $0 <perf|valgrind> [options]

Options:
  -g CMD   Middle UCI command   (default: "go wtime 1000 btime 1000")
  -b PATH  Engine binary        (default: bin/filipbot_profile)
  -o DIR   Output directory     (default: .cache/filipbot-profiles)
  -t SECS  Max seconds to wait for "bestmove" before sending "quit" anyway
           (default: 300 for perf, 3600 for valgrind)
  -a SYM   Symbol to annotate (perf only), e.g. "MovePicker::next"
           (default: the hottest symbol)
  -n       Don't open the viewer afterwards
  -h       Show this help

Sequence sent to the engine:
  position startpos
  <-g command>
  (wait until the engine prints "bestmove")
  quit

Always written to the output directory:
  perf:     perf_*.data, report_*.txt, annotate_*.txt
  valgrind: callgrind_*.out, callgrind_annotate_*.txt

Examples:
  $0 perf
  $0 perf -g "go depth 14"
  $0 perf -g "go depth 14" -a "MovePicker::next"
  $0 valgrind -g "go depth 8"
EOF
}

[ $# -ge 1 ] || { usage; exit 1; }
case "$1" in -h|--help) usage; exit 0;; esac

TYPE=$1; shift
case "$TYPE" in
  perf|valgrind) ;;
  *) echo "Error: type must be 'perf' or 'valgrind', got '$TYPE'" >&2; usage; exit 1;;
esac

GO_CMD="go wtime 1000 btime 1000"
BIN="bin/filipbot_profile"
OUT_DIR=".cache/filipbot-profiles"
TIMEOUT=""
ANNOTATE=""
VIEW=1

while getopts "g:b:o:t:a:nh" opt; do
  case "$opt" in
    g) GO_CMD=$OPTARG ;;
    b) BIN=$OPTARG ;;
    o) OUT_DIR=$OPTARG ;;
    t) TIMEOUT=$OPTARG ;;
    a) ANNOTATE=$OPTARG ;;
    n) VIEW=0 ;;
    h) usage; exit 0 ;;
    *) usage; exit 1 ;;
  esac
done

if [ -z "$TIMEOUT" ]; then
  [ "$TYPE" = perf ] && TIMEOUT=300 || TIMEOUT=3600
fi

[ -x "$BIN" ] || { echo "Error: '$BIN' not found or not executable. Build with: make type=profile" >&2; exit 1; }
command -v "$TYPE" >/dev/null || { echo "Error: '$TYPE' is not installed" >&2; exit 1; }

mkdir -p "$OUT_DIR"
STAMP=$(date +%Y%m%d_%H%M%S)
LOG="$OUT_DIR/engine_${TYPE}_${STAMP}.log"
: > "$LOG"   # create now so the feeder can poll it immediately

# Commands sent to the engine's stdin. After the go command, poll the
# engine's output log until "bestmove" shows up (or the timeout hits),
# then send quit.
feed() {
  echo "position startpos"
  echo "$GO_CMD"
  local start=$SECONDS
  while ! grep -q '^bestmove' "$LOG"; do
    if [ $((SECONDS - start)) -ge "$TIMEOUT" ]; then
      echo "Warning: no bestmove after ${TIMEOUT}s, sending quit anyway" >&2
      break
    fi
    sleep 0.2
  done
  echo "quit"
}

echo "Type:    $TYPE"
echo "Binary:  $BIN"
echo "Command: $GO_CMD (wait for bestmove, timeout ${TIMEOUT}s)"
echo "Output:  $OUT_DIR"

case "$TYPE" in
  perf)
    OUT="$OUT_DIR/perf_${STAMP}.data"
    # Precise sampling (low skid). Falls back to less precise events if the
    # machine (e.g. a VM) rejects :pp.
    PERF_EVENT=""
    for ev in cycles:pp cycles:p cycles; do
      if perf record -e "$ev" -o /dev/null -- true >/dev/null 2>&1; then
        PERF_EVENT=$ev
        break
      fi
    done
    [ -n "$PERF_EVENT" ] || { echo "Error: perf cannot record any cycles event here" >&2; exit 1; }
    echo "Event:   $PERF_EVENT"
    feed | perf record -e "$PERF_EVENT" -F 4999 --call-graph fp -o "$OUT" -- "$BIN" >"$LOG" 2>&1 || true
    ;;
  valgrind)
    OUT="$OUT_DIR/callgrind_${STAMP}.out"
    feed | valgrind --tool=callgrind --dump-instr=yes --collect-jumps=yes \
             --callgrind-out-file="$OUT" "$BIN" >"$LOG" 2>&1 || true
    ;;
esac

if [ ! -s "$OUT" ]; then
  echo "Error: no profile was produced. Check the log: $LOG" >&2
  exit 1
fi

echo "Profile saved: $OUT"
echo "Engine log:    $LOG"

# ---- Text reports (always written, even with -n) ----
case "$TYPE" in
  perf)
    REPORT="$OUT_DIR/report_${STAMP}.txt"
    ANNOT="$OUT_DIR/annotate_${STAMP}.txt"
    perf report -i "$OUT" --no-children --stdio 2>/dev/null | head -n 60 > "$REPORT" || true
    # ${ANNOTATE:+...} adds -s only when -a was given; otherwise perf
    # annotates the hottest symbol.
    perf annotate -i "$OUT" --stdio -l ${ANNOTATE:+-s "$ANNOTATE"} > "$ANNOT" 2>&1 || true
    echo "Report:        $REPORT"
    echo "Annotation:    $ANNOT"
    ;;
  valgrind)
    ANNOT="$OUT_DIR/callgrind_annotate_${STAMP}.txt"
    callgrind_annotate --auto=yes --inclusive=no "$OUT" > "$ANNOT" 2>&1 || true
    echo "Annotation:    $ANNOT"
    ;;
esac

[ "$VIEW" -eq 1 ] || exit 0

case "$TYPE" in
  perf)
    if command -v hotspot >/dev/null; then
      nohup hotspot "$OUT" >/dev/null 2>&1 &
    else
      perf report --no-children -i "$OUT"
    fi
    ;;
  valgrind)
    if command -v kcachegrind >/dev/null; then
      nohup kcachegrind "$OUT" >/dev/null 2>&1 &
    else
      echo "kcachegrind not found; showing text summary instead."
      head -n 60 "$ANNOT"
    fi
    ;;
esac