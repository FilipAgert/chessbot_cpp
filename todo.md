# Toolkits

- Make a new executeable which directly runs searches. We can track depth reached, quiesence depth reached, total nodes evaluated and time taken.
    Do this for a bunch of FENs.
    This lets us track if e.g. a search change is helpful or not.
    It is different from move generation tracking as this also takes into account the search.

# Threads

Low priority.

- Have UCI thread seperate from game search thread. Have it constantly poll for info messages and e.g. be able to cancel a search if necessary.
- Multithreading for search (low prio)

# Search

High priority

- Save move generation if possible. We can try to use TT or killer moves first - if legal.
    The savings may be negligble as the move generation is fairly quick.
- Futility pruning <https://chessprogramming.org/Futility_Pruning>
    Low priority.
    Even in futility pruned, still search moves that check, captures.
    Do not use when in check, or when alpha/beta is close to mate value.
    Need to check that there exists a legal move.
- Late move reductions (LMR) <https://chessprogramming.org/Late_Move_Reductions>
    Search first moves (1 or 2) to full depth. If no moves fail high, reduce the depth of the remaining moves.
    Do not reduce depth on:
        Captures / promotions
        When in check
        Moves giving check
    Re-searching:
        If the reduced move returns a score above alpha re-search at full depth.

## Move ordering

Medium priority

- Static exchange evaluation <https://chessprogramming.org/Static_Exchange_Evaluation>
    Sees the likely exchange of material when chain taking.
- Mate killers <https://chessprogramming.org/Mate_Killers>
    DO NOT IMPLEMENT: -ELO: -17 +/- 37. 18% chance of being better.
    Causing a beta-cutoff with value saying we will get a mate. These are ranked higher than killers.
- Order even losing captures above all else. Currently we may give a negative score. This can be avoided. May not help though.

# Evaluation

Medium priority

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
