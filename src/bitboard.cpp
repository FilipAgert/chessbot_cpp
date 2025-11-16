#include <bitboard.h>
using namespace dirs;
using namespace masks;
namespace BitBoard{
    uint64_t knight_moves(const uint64_t knight_loc, const uint64_t friendly_bb){
        //Gets all possible attack squares a knight can make by shifting bitboard a few times.
        uint64_t out = (knight_loc & (~(top | row(6)))    & (~right))          << (N+N+E); //NNE
        out |=         (knight_loc & (~(top))             & (~(right|col(6)))) << (N+E+E); //NEE
        out |=         (knight_loc & (~(bottom))          & (~(right|col(6)))) >> -(S+E+E); //SEE
        out |=         (knight_loc & (~(bottom | row(1))) & (~(right)))        >> -(S+S+E); //SSE
                                                                              
        out |=         (knight_loc & (~(bottom | row(1))) & (~(left)))         >> -(S+S+W); //SSW
        out |=         (knight_loc & (~(bottom))          & (~(left|col(1))))  >> -(S+W+W); //SWW
        out |=         (knight_loc & (~(top))             & (~(left|col(1))))  << (N+W+W); //NWW
        out |=         (knight_loc & (~(top | row(6)))    & (~left))           << (N+N+W); //NNW
        return out & ~friendly_bb;
    }
    uint64_t ray(const uint64_t origin, const int dir, const uint64_t blocker_bb, int steps){
        //How to ensure no wrap-around?
        uint64_t hit = origin;
        uint64_t mask = edge_mask(dir) | blocker_bb; //The edge mask ensures we do not wrap-around.The blocker mask ensures that we do not keep going through somebody.
        for (int i =1; i<=steps; i++){
            hit |= shift_bb((~mask)&hit, dir);//Shift the mask in dir direction, but only on non-masked places.
        }
        return hit& ~origin;//Exclude origin, since the piece does not attack itself.
    }
    uint64_t ray(const uint64_t origin, const int dir, const uint64_t blocker_bb){
        return ray(origin, dir, blocker_bb, 7);
    }
    uint64_t ray(const uint64_t origin, const int dir){
        return ray(origin, dir, 0, 7);
    }
    uint64_t rook_moves(const uint64_t rook_bb, const uint64_t friendly_bb, const uint64_t enemy_bb){
        uint64_t all = friendly_bb | enemy_bb;
        uint64_t hit = ray(rook_bb, N, all);
        hit |= ray(rook_bb, E, all);
        hit |= ray(rook_bb, S, all);
        hit |= ray(rook_bb, W, all);
        return hit & ~friendly_bb; 
    }
    uint64_t bishop_moves(const uint64_t bishop_bb, const uint64_t friendly_bb, const uint64_t enemy_bb){
        uint64_t all = friendly_bb | enemy_bb;
        uint64_t hit = ray(bishop_bb, NE, all);
        hit |= ray(bishop_bb, SE, all);
        hit |= ray(bishop_bb, SW, all);
        hit |= ray(bishop_bb, NW, all);
        return hit & ~friendly_bb; 
    }
    uint64_t queen_moves(const uint64_t queen_bb, const uint64_t friendly_bb, const uint64_t enemy_bb){
        return rook_moves(queen_bb, friendly_bb, enemy_bb) | bishop_moves(queen_bb, friendly_bb, enemy_bb); 
    }
    uint64_t king_moves(const uint64_t king_bb, const uint64_t friendly_bb){
        uint64_t hit = ray(king_bb, N, 1);
        hit |= ray(king_bb, NE, 1);
        hit |= ray(king_bb, E, 1);
        hit |= ray(king_bb, SE, 1);
        hit |= ray(king_bb, S, 1);
        hit |= ray(king_bb, SW, 1);
        hit |= ray(king_bb, W, 1);
        hit |= ray(king_bb, NW, 1);
        return hit & ~friendly_bb;
    }
}

namespace masks{
    
    uint64_t edge_mask(int dir){
        uint64_t outmask = 0;
        switch(dir){
            case E: outmask = right; break;
            case W: outmask = left; break;
            case S: outmask = bottom; break;
            case N: outmask = top; break;
            case NE: outmask = top | right; break;
            case NW: outmask = top | left; break;
            case SE: outmask = bottom | right; break;
            case SW: outmask = bottom | left; break;
            default: 
                std::cerr << "err: wrong dir" << std::endl; 
                abort();
                break;
        }
        return outmask;
    }
}