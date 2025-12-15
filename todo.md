# Search
- Quiesence search
    Add king in check.
- King in check search
    - Captures
    - King moves
    - Blocking
- King in double check search
    - Only king moves

- Move ordering by simple heuristic. Eg. attacking piece.
# Board representation
dont put bitboards in array. put them raw. No need for bitboard for king either.

To speedup move generation:
1. Remove 8x8 board containing pieces
Likely also for the generating bitboards etc.

When doing a move, also add auto king check detection. This can be STORED in the board state. Why? To be accessed by e.g. eval.
    This should detect: 1 check, 2 or more check.

Can be used in movegeneration.
If 1 check:
    Capture offending piece
    Blocking ray
    King move
If 2 check:
    King move.

Check for pinned king.
    Generate a function xray rook and xray bishop attacks which is just a magic bitboard but it ignores the first occupancy square.
    We the AND this with the enemy slider:
    If its grt 0:
    There is a piece in between us and the slider -> get a precomputed mask which is all squares between the enemy piece (inclusive) and the king piece (exclusive).
    AND this with the friendly pieces to obtain location of pinned piece.
    This pinned piece is ONLY allowed to move along the precomputed mask.

This will allow us to reduce the do/undo move in move generation to the following cases:
    EP capture>
    King moves.
    King in check.


## Magic bitboards:
Try to generate magic numbers which reduce the span of the upper bound in the hash region.
I.e. try to minimize
[start, stop].
In practice, there will always be a key at start (the empty occ board will map to zero, so minimise the stop.
The upper index can be hardcoded into an array (the sizes) which are to be used for computing the offsets.

Can also try black magic bitboards.

-Remove the check checking and adding to move to aid move ordering in search.

-Can try in atk mode to loop over pieces, find all sliding pieces and run the lookup individually straight on them. maybe less overhead than looping through the bitboard.
