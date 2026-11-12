import chess
import chess.engine
import os

# ================= CONFIGURATION =================

INPUT_FILE = "data/lichess/all.fen"
OUTPUT_FILE = "data/lichess/all_filtered.fen"

STOCKFISH_PATH = r"stockfish"  # Change this!

EVAL_LIMIT = 2.0
DEPTH = 10

PROGRESS_INTERVAL = 10

# =================================================


def is_quiet_position(board, engine):
    """
    Check the next two Stockfish best moves.
    Reject if either move is a capture or promotion.
    """

    temp_board = board.copy()

    for _ in range(2):

        result = engine.play(
            temp_board,
            chess.engine.Limit(depth=DEPTH)
        )

        move = result.move

        if move is None:
            break

        # Reject captures
        if temp_board.is_capture(move):
            return False

        # Reject promotions
        if move.promotion is not None:
            return False

        temp_board.push(move)

    return True


def evaluate_position(board, engine):
    """
    Evaluate position from White's perspective.
    Returns evaluation in pawns.
    """

    info = engine.analyse(
        board,
        chess.engine.Limit(depth=DEPTH)
    )

    score = info["score"].white()

    if score.is_mate():
        return None

    return score.score() / 100.0


def main():

    if not os.path.exists(INPUT_FILE):
        print(f"Input file not found: {INPUT_FILE}")
        return

    engine = chess.engine.SimpleEngine.popen_uci(STOCKFISH_PATH)

    total_read = 0
    pruned = 0
    remaining = 0

    try:

        with open(INPUT_FILE, "r", encoding="utf-8") as infile, \
             open(OUTPUT_FILE, "w", encoding="utf-8") as outfile:

            for line in infile:

                line = line.strip()

                if not line:
                    continue

                total_read += 1

                try:

                    # Expected format: FEN, result
                    fen, result = line.rsplit(",", 1)

                    fen = fen.strip()
                    result = result.strip()

                    board = chess.Board(fen)

                    # Reject positions where either side is in check
                    if board.is_check():
                        pruned += 1
                        continue

                    # Reject positions where evaluation exceeds +/- 2
                    evaluation = evaluate_position(board, engine)

                    if evaluation is None or abs(evaluation) > EVAL_LIMIT:
                        pruned += 1
                        continue

                    # Reject positions with captures/promotions
                    # in Stockfish's next two best moves
                    if not is_quiet_position(board, engine):
                        pruned += 1
                        continue

                    # Keep position
                    outfile.write(f"{fen}, {result}, {evaluation}\n")
                    remaining += 1

                except Exception as e:
                    pruned += 1
                    print(f"Error on line {total_read}: {e}")

                # Progress report
                if total_read % PROGRESS_INTERVAL == 0:
                    print(
                        f"Read: {total_read:,} | "
                        f"Pruned: {pruned:,} | "
                        f"Remaining: {remaining:,}"
                    )

    finally:
        engine.quit()

    print("\n========== FINISHED ==========")
    print(f"Total read:  {total_read:,}")
    print(f"Pruned:      {pruned:,}")
    print(f"Remaining:   {remaining:,}")
    print(f"Output file: {OUTPUT_FILE}")


if __name__ == "__main__":
    main()
