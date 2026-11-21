# Toolkits

- Make a new executeable which directly runs searches. We can track depth reached, quiesence depth reached, total nodes evaluated and time taken.
    Do this for a bunch of FENs.
    This lets us track if e.g. a search change is helpful or not.
    It is different from move generation tracking as this also takes into account the search.

# Time management

Manage time better

- If stable search, use less time
  cp stable (< 20 cp change)
  PV stable over several iterations
  If high effort spent in PV node, can use as signal (e.g. mark effort in each node by order in root search.)
  Do not allow when root is failing low.
  
- If unstable search, extend
  PV changed or cp changed by > 20, extend time

# Search

High priority

- Save move generation if possible. We can try to use TT or killer moves first - if legal.
    The savings may be negligble as the move generation is fairly quick.

## Quiesence

- Null move pruning?

## Move ordering

- Mate killers <https://chessprogramming.org/Mate_Killers>
    DO NOT IMPLEMENT: -ELO: -17 +/- 37. 18% chance of being better.
    Causing a beta-cutoff with value saying we will get a mate. These are ranked higher than killers.

# Evaluation

High priority:

- Pawn structure eval. Use a transposition table keyed only on pawns for this.
  - Update hash incrementally.
- Full tapered eval.

- Rooks on open files / doubled rooks

- King safety
  - Pawn shield only for castled king
  - Open files near king bad

Medium priority

Make eval handle a loss of material better:
If only opponent has material to win:
clamp score in ]-inf, -1]
this gives us incentive to draw.
E.g. one game I saw we had knight vs pawn.
We can never win, so its worth it to sack the knight for the pawn.

- Replace eval by neural network.

1. Get a ton of lichess games - split into train and verification datasets.
2. Take positions from these games as data & score it
3. See result of game as evaluator.
4. Symmetric in white/black - can reduce number of weights. Can probably contexpr this as well.

# Move generation

Low priority.

To speedup move generation:

1. Benchmark removing 8x8 board containing pieces
Likely also for the generating bitboards etc.

## Magic bitboards

Try to generate magic numbers which reduce the span of the upper bound in the hash region.
I.e. try to minimize
[start, stop].
In practice, there will always be a key at start (the empty occ board will map to zero, so minimise the stop.
The upper index can be hardcoded into an array (the sizes) which are to be used for computing the offsets.

Can also try black magic bitboards.

-Remove the check checking and adding to move to aid move ordering in search.

-Can try in atk mode to loop over pieces, find all sliding pieces and run the lookup individually straight on them. maybe less overhead than looping through the bitboard.
