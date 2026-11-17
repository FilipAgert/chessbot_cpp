#include "search/state_history.h"

int StateStack::count_elem(HashEntry hash) const {
    int ctr = 0;
    int num_pieces = hash.num_pieces;
    for (int i = std::min(top_idx, max_size - 1); i >= 0; i--) {
        if (stack[i].num_pieces < num_pieces)
            break;
        ctr += stack[i] == hash;
    }
    return ctr;
}
bool StateStack::is_repetition(int root_idx) const {
    const HashEntry &cur = stack[top_idx];
    int before_root = 0;
    assert(root_idx >= 0);
    for (int i = top_idx - 1; i >= 0; i--) {
        if (stack[i].num_pieces < cur.num_pieces)
            break; // capture happened, can't repeat further back
        if (stack[i] == cur) {
            if (i >= root_idx)
                return true; // repeated inside search
            if (++before_root >= 2)
                return true; // true 3-fold with history
        }
    }
    return false;
}

/**
 * @brief Checks if there are atleast some number of occurences or more in the stack
 *
 * @param[in] hash hash to check for
 * @param[in] num number of hashes in stack
 * @return true if there are at least num hashes of this hash in the stack.
 */
int StateStack::atleast_num(HashEntry hash, int num) {
    int ctr = 0;
    int num_pieces = hash.num_pieces;
    for (int i = std::min(top_idx, max_size - 1); i >= 0; i--) {
        if (stack[i].num_pieces < num_pieces)
            break;
        ctr += stack[i] == hash;
        if (ctr >= num)
            return true;
    }
    return false;
}