#!/usr/bin/env python3
"""
pgn_to_fen.py - Filter games in large (optionally compressed) PGN files and
export board states as "<fen>, <result>" lines.

Requires:
    pip install chess
    Ubuntu tools as needed:  sudo apt install zstd xz-utils bzip2 gzip tar unzip p7zip-full lz4

Supported inputs (detected by file name):
    .pgn                                   read directly
    .zst .zstd .gz .bz2 .xz .lzma .lz4     single-file compressors
    .tar .tar.gz .tgz .tar.bz2 .tbz2 .tar.xz .txz .tar.zst .tzst .tar.lz4
                                           tar archives (every *.pgn inside is processed)
    .zip .7z                               archives (every *.pgn inside is processed)

Workflow, per input (processed one after the other):
    1. Decompress to a temp dir (or read directly if it is a plain .pgn)
    2. Pass 1: scan headers, apply filters, remember offsets of selected games
       Pass 2: (after confirmation) parse the selected games and write positions
    3. Delete the decompressed copy; the original archive stays compressed
    4. Go to the next input

Output format:
    With --mode all / fraction, each game's positions are preceded by a separator:
        =======GAME 1=======
        <fen>, <result>
        <fen>, <result>
        =======GAME 2=======
        ...
    With --mode one (a single position per game) no separators are written.
    Game numbers keep counting across all games and input files.

Examples:
    python pgn_to_fen.py -o out.txt lichess_2023-01.pgn.zst --mode fraction --fraction 0.2 --seed 42
    python pgn_to_fen.py -o out.txt a.pgn.zst b.pgn.xz c.tar.gz --yes
    python pgn_to_fen.py a.pgn.zst --dry-run
    python pgn_to_fen.py -o out.txt big.pgn.zst --tmp-dir /mnt/bigdisk/tmp
"""

from __future__ import annotations

import argparse
import contextlib
import os
import random
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from typing import Iterable, Iterator, Optional

import chess
import chess.pgn

# --------------------------------------------------------------------------- #
# Constants / configuration
# --------------------------------------------------------------------------- #

RESULT_MAP = {"1-0": 1, "0-1": -1, "1/2-1/2": 0}  # "*" and anything else -> skipped

MODE_ALL = "all"            # Mode 1: every board state
MODE_ONE = "one"            # Mode 2: exactly one random board state per game
MODE_FRACTION = "fraction"  # Mode 3: random fraction of board states per game


@dataclass
class FilterConfig:
    """All filters are optional; None / empty means 'no restriction'."""
    min_white_elo: Optional[int] = None
    max_white_elo: Optional[int] = None
    min_black_elo: Optional[int] = None
    max_black_elo: Optional[int] = None
    # Allowed values (case-insensitive). Empty set = allow everything.
    terminations: set[str] = field(default_factory=set)
    max_time: Optional[int] = None


@dataclass
class SamplingConfig:
    mode: str = MODE_ALL
    fraction: float = 0.2                   # used only in MODE_FRACTION
    include_initial_position: bool = False  # also emit the position before move 1
    skip_games_with_errors: bool = True     # skip games with illegal/unparsable moves


# --------------------------------------------------------------------------- #
# Compression handling
# --------------------------------------------------------------------------- #

# Single-stream compressors: suffix -> command that writes decompressed data to stdout.
# --long=31 is needed for big zstd files (e.g. Lichess dumps use a 2 GB window).
STREAM_CMDS: dict[str, list[str]] = {
    ".zst":   ["zstd", "-d", "-c", "-q", "--long=31"],
    ".zstd":  ["zstd", "-d", "-c", "-q", "--long=31"],
    ".gz":    ["gzip", "-dc"],
    ".bz2":   ["bzip2", "-dc"],
    ".xz":    ["xz", "-dc"],
    ".lzma":  ["xz", "--format=lzma", "-dc"],
    ".lz4":   ["lz4", "-dc", "-q"],
}

TAR_SUFFIXES = (".tar", ".tar.gz", ".tgz", ".tar.bz2", ".tbz2", ".tar.xz", ".txz",
                ".tar.zst", ".tzst", ".tar.lz4", ".tar.lzma")


def detect_kind(path: str) -> str:
    """Return one of: 'plain', 'tar', 'zip', '7z', 'stream'."""
    name = path.lower()
    if name.endswith(TAR_SUFFIXES):
        return "tar"
    if name.endswith(".zip"):
        return "zip"
    if name.endswith(".7z"):
        return "7z"
    if any(name.endswith(s) for s in STREAM_CMDS):
        return "stream"
    return "plain"


def _require(tool: str) -> None:
    if shutil.which(tool) is None:
        raise RuntimeError(f"'{tool}' not found. Install it, e.g.: sudo apt install {tool}")


def _find_pgns(root: str) -> list[str]:
    found = []
    for dirpath, _, files in os.walk(root):
        for f in files:
            if f.lower().endswith(".pgn"):
                found.append(os.path.join(dirpath, f))
    return sorted(found)


@contextlib.contextmanager
def prepared_pgns(path: str, tmp_dir: Optional[str] = None,
                  keep: bool = False) -> Iterator[list[str]]:
    """
    Yield a list of readable .pgn file paths for the given input.
    Compressed inputs are decompressed into a temp dir, which is removed on exit
    (the original archive is never touched). Plain .pgn files are yielded as is.
    """
    kind = detect_kind(path)
    if kind == "plain":
        yield [path]
        return

    work = tempfile.mkdtemp(prefix="pgn_", dir=tmp_dir)
    try:
        print(f"Decompressing {path} -> {work} ...", file=sys.stderr)
        if kind == "stream":
            suffix = next(s for s in STREAM_CMDS if path.lower().endswith(s))
            cmd = STREAM_CMDS[suffix]
            _require(cmd[0])
            base = os.path.basename(path)[: -len(suffix)] or "decompressed"
            if not base.lower().endswith(".pgn"):
                base += ".pgn"
            target = os.path.join(work, base)
            with open(target, "wb") as out:
                subprocess.run(cmd + [path], stdout=out, check=True)
            pgns = [target]
        else:
            if kind == "tar":
                _require("tar")
                subprocess.run(["tar", "-xf", path, "-C", work], check=True)
            elif kind == "zip":
                _require("unzip")
                subprocess.run(["unzip", "-qq", "-o", path, "-d", work], check=True)
            else:  # 7z
                _require("7z")
                subprocess.run(["7z", "x", "-y", f"-o{work}", path],
                               check=True, stdout=subprocess.DEVNULL)
            pgns = _find_pgns(work)
            if not pgns:
                print(f"[warn] no .pgn files found inside {path}", file=sys.stderr)
        yield pgns
    finally:
        if keep:
            print(f"Keeping decompressed files in {work}", file=sys.stderr)
        else:
            shutil.rmtree(work, ignore_errors=True)
            print(f"Removed decompressed copy of {path}", file=sys.stderr)


# --------------------------------------------------------------------------- #
# Header helpers
# --------------------------------------------------------------------------- #

def parse_elo(value: Optional[str]) -> Optional[int]:
    """Convert an Elo header to int; return None for missing/unknown ('?', '-', '')."""
    if value is None:
        return None
    value = value.strip()
    if not value or value in {"?", "-", "*"}:
        return None
    try:
        return int(value)
    except ValueError:
        try:  # e.g. "1500.0"
            return int(float(value))
        except ValueError:
            return None


def extract_fields(headers: chess.pgn.Headers) -> dict:
    """Pull out the fields we care about from a game's headers."""
    return {
        "result": headers.get("Result", "*").strip(),
        "white_elo": parse_elo(headers.get("WhiteElo")),
        "black_elo": parse_elo(headers.get("BlackElo")),
        "time_control": headers.get("TimeControl", "").strip(),
        "termination": headers.get("Termination", "").strip(),
    }


def _elo_ok(elo: Optional[int], lo: Optional[int], hi: Optional[int]) -> bool:
    if lo is None and hi is None:
        return True            # no restriction -> unknown Elo is fine
    if elo is None:
        return False           # restriction requested but Elo unknown -> reject
    return (lo is None or elo >= lo) and (hi is None or elo <= hi)


def game_passes_filters(fields: dict, flt: FilterConfig) -> bool:
    """True if the game has a usable result and satisfies every active filter."""
    if fields["result"] not in RESULT_MAP:
        return False
    if not _elo_ok(fields["white_elo"], flt.min_white_elo, flt.max_white_elo):
        return False
    if not _elo_ok(fields["black_elo"], flt.min_black_elo, flt.max_black_elo):
        return False
    if flt.terminations and fields["termination"].lower() not in flt.terminations:
        return False
    if flt.max_time:
        # time control as <base>+<increment>
        try:
            base = int(fields["time_control"].split("+")[0])
            if base > flt.max_time:
                return False
        except Exception:
            return False
    return True


# --------------------------------------------------------------------------- #
# Pass 1: select games (headers only)
# --------------------------------------------------------------------------- #

def select_games(pgn_path: str, flt: FilterConfig,
                 progress_every: int = 100_000) -> tuple[list[int], int]:
    """
    Scan the PGN reading only headers. Returns (offsets, total_games_seen)
    where offsets are file positions of the games that pass the filters.
    """
    offsets: list[int] = []
    total = 0
    with open(pgn_path, "r", encoding="utf-8", errors="replace", newline="") as fh:
        while True:
            offset = fh.tell()
            try:
                headers = chess.pgn.read_headers(fh)  # also skips the movetext
            except Exception as exc:  # malformed chunk; keep going
                print(f"[warn] header read error near byte {offset}: {exc}", file=sys.stderr)
                continue
            if headers is None:  # EOF
                break
            total += 1
            if game_passes_filters(extract_fields(headers), flt):
                offsets.append(offset)
            if progress_every and total % progress_every == 0:
                print(f"  scanned {total:,} games, {len(offsets):,} selected...", file=sys.stderr)
    return offsets, total


# --------------------------------------------------------------------------- #
# Pass 2: sample board states and write output
# --------------------------------------------------------------------------- #

def game_fens(game: chess.pgn.Game, include_initial: bool) -> list[str]:
    """Return the FEN after every move (optionally also the starting position)."""
    board = game.board()  # honours [FEN]/[SetUp] headers
    fens = [board.fen()] if include_initial else []
    for move in game.mainline_moves():
        board.push(move)
        fens.append(board.fen())
    return fens


def sample_fens(fens: list[str], cfg: SamplingConfig, rng: random.Random) -> list[str]:
    """Apply the chosen sampling mode to a game's list of FENs."""
    n = len(fens)
    if n == 0:
        return []
    if cfg.mode == MODE_ALL:
        return fens
    if cfg.mode == MODE_ONE:
        return [fens[rng.randrange(n)]]
    if cfg.mode == MODE_FRACTION:
        k = max(1, round(n * cfg.fraction))            # at least one state per game
        idx = sorted(rng.sample(range(n), min(k, n)))  # keep game order
        return [fens[i] for i in idx]
    raise ValueError(f"Unknown sampling mode: {cfg.mode}")


def write_positions(pgn_path: str, offsets: Iterable[int], dst,
                    cfg: SamplingConfig, rng: random.Random,
                    first_game_number: int = 1,
                    progress_every: int = 50_000) -> tuple[int, int, int]:
    """
    Re-read each selected game, sample states and write them to the open file `dst`.
    Unless the mode is MODE_ONE, each game's block starts with '=======GAME n======='.
    Returns (games_written, positions_written, games_skipped).
    """
    games_written = positions_written = skipped = 0
    use_separator = cfg.mode != MODE_ONE   # one position per game -> no separator needed

    with open(pgn_path, "r", encoding="utf-8", errors="replace", newline="") as src:
        for i, offset in enumerate(offsets, 1):
            src.seek(offset)
            try:
                game = chess.pgn.read_game(src)
            except Exception as exc:
                print(f"[warn] could not parse game at byte {offset}: {exc}", file=sys.stderr)
                skipped += 1
                continue
            if game is None:
                skipped += 1
                continue
            if cfg.skip_games_with_errors and game.errors:
                skipped += 1
                continue

            result = RESULT_MAP.get(game.headers.get("Result", "*").strip())
            if result is None:  # should not happen (pre-filtered) but be safe
                skipped += 1
                continue

            try:
                fens = game_fens(game, cfg.include_initial_position)
            except Exception as exc:  # e.g. invalid custom start FEN
                print(f"[warn] bad game at byte {offset}: {exc}", file=sys.stderr)
                skipped += 1
                continue

            chosen = sample_fens(fens, cfg, rng)
            if not chosen:
                skipped += 1
                continue

            if use_separator:
                dst.write(f"=======GAME {first_game_number + games_written}=======\n")
            for fen in chosen:
                dst.write(f"{fen}, {result}\n")
            games_written += 1
            positions_written += len(chosen)

            if progress_every and i % progress_every == 0:
                print(f"  processed {i:,} games, {positions_written:,} positions...",
                      file=sys.stderr)
    return games_written, positions_written, skipped


# --------------------------------------------------------------------------- #
# High-level API + CLI
# --------------------------------------------------------------------------- #

def _confirm(out_path: str, label: str, state: dict) -> bool:
    """Ask whether to save. 'a' = yes for this and all remaining files."""
    if state["yes"]:
        return True
    if not sys.stdin.isatty():
        print("Non-interactive session: use --yes to save without prompting.")
        return False
    answer = input(f"Save positions from '{label}' to '{out_path}'? [y/N/a(ll)] ").strip().lower()
    if answer in {"a", "all"}:
        state["yes"] = True
        return True
    return answer in {"y", "yes"}


def run(inputs: list[str], out_path: Optional[str], flt: FilterConfig, cfg: SamplingConfig,
        seed: Optional[int] = None, assume_yes: bool = False, dry_run: bool = False,
        tmp_dir: Optional[str] = None, keep_decompressed: bool = False) -> None:
    rng = random.Random(seed)
    state = {"yes": assume_yes}
    next_game = 1                       # global game counter across all files
    grand_games = grand_pos = grand_skipped = 0

    with contextlib.ExitStack() as stack:
        dst = None                      # output file is opened lazily on first write

        for input_path in inputs:
            print(f"\n=== {input_path} ===")
            if not os.path.isfile(input_path):
                print(f"[warn] not a file, skipping: {input_path}", file=sys.stderr)
                continue

            try:
                # 1. decompress (or read directly) ... 3. cleanup happens on exit of `with`
                with prepared_pgns(input_path, tmp_dir, keep_decompressed) as pgns:
                    for pgn in pgns:
                        label = input_path if len(pgns) == 1 and pgn == input_path \
                            else f"{input_path}:{os.path.basename(pgn)}"

                        # 2a. gather data (pass 1)
                        print(f"Scanning {label} ...", file=sys.stderr)
                        offsets, total = select_games(pgn, flt)
                        print(f"Games in file: {total:,}")
                        print(f"Games selected after filtering: {len(offsets):,}")

                        if dry_run or not offsets:
                            continue
                        if not _confirm(out_path, label, state):
                            print("Nothing written for this file.")
                            continue

                        # 2b. write positions (pass 2)
                        if dst is None:
                            dst = stack.enter_context(
                                open(out_path, "w", encoding="utf-8", newline="\n"))
                        g, p, s = write_positions(pgn, offsets, dst, cfg, rng,
                                                  first_game_number=next_game)
                        dst.flush()
                        next_game += g
                        grand_games += g
                        grand_pos += p
                        grand_skipped += s
                        print(f"Written: {g:,} games, {p:,} positions "
                              f"(skipped {s:,} games with parse errors/empty)")
            except (subprocess.CalledProcessError, RuntimeError) as exc:
                print(f"[error] failed to process {input_path}: {exc}", file=sys.stderr)
                continue    # 4. go to next input

    if not dry_run:
        print(f"\nDone. Games written: {grand_games:,} | positions written: {grand_pos:,} "
              f"| games skipped: {grand_skipped:,}")
        if grand_games:
            print(f"Output file: {out_path}")


def build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Extract FEN positions + results from (compressed) PGN files.")
    p.add_argument("inputs", nargs="+",
                   help="input .pgn or compressed archive(s) (.zst, .xz, .gz, .bz2, .tar.*, .zip, .7z)")
    p.add_argument("-o", "--output",
                   help="output file (.txt/.csv); required unless --dry-run")
    p.add_argument("--tmp-dir",
                   help="where to decompress archives (default: system temp dir; "
                        "make sure it has enough free space)")
    p.add_argument("--keep-decompressed", action="store_true",
                   help="do not delete the decompressed copy after processing")

    g = p.add_argument_group("filters")
    g.add_argument("--min-white-elo", type=int, default=2000)
    g.add_argument("--max-white-elo", type=int, default=10000)
    g.add_argument("--min-black-elo", type=int, default=2000)
    g.add_argument("--max-black-elo", type=int, default=10000)
    g.add_argument("--terminations", nargs="*", default=["Normal"],
                   help='allowed Termination values, e.g. Normal "Time forfeit"')
    g.add_argument("--max_time", type=int, default=15,
                   help="allowed maximum base time in the TimeControl header")

    s = p.add_argument_group("sampling")
    s.add_argument("--mode", choices=[MODE_ALL, MODE_ONE, MODE_FRACTION], default=MODE_ALL,
                   help="all = every state; one = 1 random state/game; "
                        "fraction = random fraction of states/game")
    s.add_argument("--fraction", type=float, default=0.2,
                   help="fraction of states per game for --mode fraction (0-1], default 0.2")
    s.add_argument("--include-initial", action="store_true",
                   help="also include the position before the first move")
    s.add_argument("--keep-error-games", action="store_true",
                   help="keep games with illegal moves (positions up to the error)")
    s.add_argument("--seed", type=int, help="random seed for reproducibility")

    p.add_argument("--yes", "-y", action="store_true", help="save without asking")
    p.add_argument("--dry-run", action="store_true", help="only count selected games")
    return p


def main() -> None:
    parser = build_arg_parser()
    args = parser.parse_args()
    if args.mode == MODE_FRACTION and not (0 < args.fraction <= 1):
        sys.exit("--fraction must be in (0, 1]")
    if not args.dry_run and not args.output:
        parser.error("-o/--output is required unless --dry-run is used")

    flt = FilterConfig(
        min_white_elo=args.min_white_elo, max_white_elo=args.max_white_elo,
        min_black_elo=args.min_black_elo, max_black_elo=args.max_black_elo,
        terminations={t.lower() for t in args.terminations},
        max_time=args.max_time,
    )
    cfg = SamplingConfig(
        mode=args.mode, fraction=args.fraction,
        include_initial_position=args.include_initial,
        skip_games_with_errors=not args.keep_error_games,
    )
    run(args.inputs, args.output, flt, cfg, seed=args.seed,
        assume_yes=args.yes, dry_run=args.dry_run,
        tmp_dir=args.tmp_dir, keep_decompressed=args.keep_decompressed)


if __name__ == "__main__":
    main()