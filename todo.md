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
# Move representation
Fit into 32 bit
6: source
6: target
4: promotion
4: captured
4: castle
Others?
# Board representation
dont put bitboards in array. put them raw. No need for bitboard for king either.

To speedup move generation.
1. Remove 8x8 board containing pieces
2. Generate template functions for move generation, color of player moved and type of move.
This will drastically speed up do and undo move.
Likely also for the generating bitboards etc.

One template for each piece moved & colored.
So move function would be
<white_moved, piece_type, move_flag> do_move(uint8_t from, uint8_t to)
undo function could be
<white_moved, piece_type, move_flag, captured_piece> undo_move(uint8_t from, uint8_t to)

E.g. when doing a move we have one flag for:
castle long / castle short
promote queen
promote knight
promote bishop
promote rook
double pawn push



## Magic bitboards:
Try to generate magic numbers which reduce the span of the upper bound in the hash region.
I.e. try to minimize
[start, stop].
In practice, there will always be a key at start (the empty occ board will map to zero, so minimise the stop.
The upper index can be hardcoded into an array (the sizes) which are to be used for computing the offsets.

Can also try black magic bitboards.

-Remove the check checking and adding to move to aid move ordering in search.

-Can try in atk mode to loop over pieces, find all sliding pieces and run the lookup individually straight on them. maybe less overhead than looping through the bitboard.
