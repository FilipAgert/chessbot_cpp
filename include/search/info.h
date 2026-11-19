#pragma once
#include "core/move.h"
#include <string>
#include <vector>
struct InfoMsg {
    int depth = 0;             // half-ply
    int seldepth = 0;          // Depth reached in quiesence
    int time = 0;              // Time elapsed since start of search
    uint64_t nodes = 0;        // Number of positions evaluated.
    int moves_generated = 0;   // Number of total moves generated.
    std::vector<Move> pv = {}; // Principal line
    Move currmove = Move();    // Current move (root) being evaluated.
    int score = 0;             // Current evaluated best move score
    int d0score = 0;           // Score of state (no going deep)
    int hashfill = 0;
    bool stringmsg = false;
    std::string string;
    float reduction_average;
    float reduction_research;
    int reduction_times;
};