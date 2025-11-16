#include <bitboard.h>
using namespace dirs;
using namespace masks;
namespace BitBoard{
    uint64_t knight_moves(const uint64_t knight_loc){
        //Gets all possible attack squares a knight can make by shifting bitboard a few times.
        uint64_t out = (knight_loc & (~(top | row(6)))    & (~right))          << (N+N+E); //NNE
        out |=         (knight_loc & (~(top))             & (~(right|col(6)))) << (N+E+E); //NEE
        out |=         (knight_loc & (~(bottom))          & (~(right|col(6)))) >> -(S+E+E); //SEE
        out |=         (knight_loc & (~(bottom | row(1))) & (~(right)))        >> -(S+S+E); //SSE
                                                                              
        out |=         (knight_loc & (~(bottom | row(1))) & (~(left)))         >> -(S+S+W); //SSW
        out |=         (knight_loc & (~(bottom))          & (~(left|col(1))))  >> -(S+W+W); //SWW
        out |=         (knight_loc & (~(top))             & (~(left|col(1))))  << (N+W+W); //NWW
        out |=         (knight_loc & (~(top | row(6)))    & (~left))           << (N+N+W); //NNW
        return out;
    }
    uint64_t ray(const uint64_t origin, const int dir){
        //How to ensure no wrap-around?
        uint64_t hit = origin;
        uint64_t mask = edge_mask(dir);
        
        for (int i =1; i<8; i++){
            hit |= shift_bb((~mask)&hit, dir);//Shift the mask in dir direction, but only on non-masked places.
        }
        return hit& ~origin;//Exclude origin, since the piece does not attack itself.
    }
    uint64_t rook_moves(const uint64_t rook_locs){
        uint64_t hit = ray(rook_locs, N);
        hit |= ray(rook_locs, E);
        hit |= ray(rook_locs, S);
        hit |= ray(rook_locs, W);
        return hit; 
    }
    uint64_t bishop_moves(const uint64_t bishop_locs){
        uint64_t hit = ray(bishop_locs, NE);
        hit |= ray(bishop_locs, SE);
        hit |= ray(bishop_locs, SW);
        hit |= ray(bishop_locs, NW);
        return hit; 
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