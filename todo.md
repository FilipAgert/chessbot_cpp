# Movegeneration

## Magic bitboards:
Try to generate magic numbers which reduce the span of the upper bound in the hash region.
I.e. try to minimize
[start, stop].
In practice, there will always be a key at start (the empty occ board will map to zero, so minimise the stop.
The upper index can be hardcoded into an array (the sizes) which are to be used for computing the offsets.

Can also try black magic bitboards.

-Remove the check checking and adding to move to aid move ordering in search.

-Can try in atk mode to loop over pieces, find all sliding pieces and run the lookup individually straight on them. maybe less overhead than looping through the bitboard.
