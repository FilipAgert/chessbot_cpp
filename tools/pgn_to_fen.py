#!/usr/bin/env python3
"""
pgn_to_fen.py - Filter games in a large PGN file and export board states
as "<fen>, <result>" lines.

Install:
    pip install chess

Workflow (two passes, so memory use stays tiny even for multi-GB files):
    Pass 1: read only the HEADERS of every game (fast), apply the filters and
            remember the file offset of each game that passes.
            -> prints how many games were selected.
    Pass 2: (only if you confirm) seek back to each selected game, parse its
            moves, sample board states and write "<fen>, <result>" lines.

Examples:
    # All positions from rated-ish blitz games, both players 1800-2200
    python pgn_to_fen.py games.pgn out.csv --mode all \
        --min-white-elo 1800 --max-white-elo 2200 \
        --min-black-elo 1800 --max-black-elo 2200 \
        --time-controls "180+0" "300+0" --terminations Normal "Time forfeit"

    # One random position per game, no confirmation prompt
    python pgn_to_fen.py games.pgn out.csv --mode one --yes

    # 20 % of the positions of each game, reproducible
    python pgn_to_fen.py games.pgn out.csv --mode fraction --fraction 0.2 --seed 42

    # Only count the games that pass the filters
    python pgn_to_fen.py games.pgn out.csv --dry-run
"""

from __future__ import annotations

import argparse
import random
import sys
from dataclasses import dataclass, field
from typing import Iterable, Optional

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
    max_time: int = None


@dataclass
class SamplingConfig:
    mode: str = MODE_ALL
    fraction: float = 0.2            # used only in MODE_FRACTION
    include_initial_position: bool = False  # also emit the position before move 1
    skip_games_with_errors: bool = True     # skip games with illegal/unparsable moves


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
        # time control as <min>+<seconds>
        try:
            minutes: int = int(fields["time_control"].split("+")[0])
            if minutes > flt.max_time:
                return False
        except:
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
    Storing one integer per selected game is very cheap in memory.
    """
    offsets: list[int] = []
    total = 0
    # newline="" keeps tell()/seek() consistent; errors="replace" survives odd bytes.
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
        k = max(1, round(n * cfg.fraction))   # at least one state per game
        idx = sorted(rng.sample(range(n), min(k, n)))  # keep game order
        return [fens[i] for i in idx]
    raise ValueError(f"Unknown sampling mode: {cfg.mode}")


def write_positions(pgn_path: str, offsets: Iterable[int], out_path: str,
                    cfg: SamplingConfig, rng: random.Random,
                    progress_every: int = 50_000) -> tuple[int, int, int]:
    """
    Re-read each selected game, sample states and write '<fen>, <result>' lines.
    Returns (games_written, positions_written, games_skipped).
    """
    games_written = positions_written = skipped = 0
    with open(pgn_path, "r", encoding="utf-8", errors="replace", newline="") as src, \
         open(out_path, "w", encoding="utf-8", newline="\n") as dst:
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

def run(pgn_path: str, out_path: str, flt: FilterConfig, cfg: SamplingConfig,
        seed: Optional[int] = None, assume_yes: bool = False,
        dry_run: bool = False) -> None:
    rng = random.Random(seed)

    print(f"Scanning {pgn_path} ...", file=sys.stderr)
    offsets, total = select_games(pgn_path, flt)
    print(f"Games in file: {total:,}")
    print(f"Games selected after filtering: {len(offsets):,}")

    if dry_run or not offsets:
        return

    # Step "6 -> 7": let the user decide whether to save.
    if not assume_yes:
        if not sys.stdin.isatty():
            print("Non-interactive session: use --yes to save without prompting.")
            return
        answer = input(f"Save positions from these games to '{out_path}'? [y/N] ").strip().lower()
        if answer not in {"y", "yes"}:
            print("Nothing written.")
            return

    games_w, pos_w, skipped = write_positions(pgn_path, offsets, out_path, cfg, rng)
    print(f"Done. Games written: {games_w:,} | positions written: {pos_w:,} "
          f"| games skipped (parse errors/empty): {skipped:,}")
    print(f"Output file: {out_path}")


def build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="Extract FEN positions + results from a PGN file.")
    p.add_argument("pgn", help="input PGN file")
    p.add_argument("output", help="output file (.txt/.csv), lines: '<fen>, <result>'")

    g = p.add_argument_group("filters")
    g.add_argument("--min-white-elo", type=int, default=2000)
    g.add_argument("--max-white-elo", type=int, default=10000)
    g.add_argument("--min-black-elo", type=int, default=2000)
    g.add_argument("--max-black-elo", type=int, default=10000)
    g.add_argument("--terminations", nargs="*", default=["Normal"],
                   help='allowed Termination values, e.g. Normal "Time forfeit"')
    g.add_argument("--max_time", type=int, default=15,
                   help='Allowed maximum time in time controls')

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
    args = build_arg_parser().parse_args()
    if args.mode == MODE_FRACTION and not (0 < args.fraction <= 1):
        sys.exit("--fraction must be in (0, 1]")

    flt = FilterConfig(
        min_white_elo=args.min_white_elo, max_white_elo=args.max_white_elo,
        min_black_elo=args.min_black_elo, max_black_elo=args.max_black_elo,
        terminations={t.lower() for t in args.terminations},
        max_time = args.max_time,
    )
    cfg = SamplingConfig(
        mode=args.mode, fraction=args.fraction,
        include_initial_position=args.include_initial,
        skip_games_with_errors=not args.keep_error_games,
    )
    run(args.pgn, args.output, flt, cfg, seed=args.seed,
        assume_yes=args.yes, dry_run=args.dry_run)


if __name__ == "__main__":
    main()