#include <bitboard.h>
using namespace dirs;
using namespace masks;
namespace BitBoard{
    uint64_t knight_moves(const uint64_t knight_loc){
        //Gets all possible attack squares a knight can make by shifting bitboard a few times.
        uint64_t out = (knight_loc & (~(top | row(6))) & (~right)) << (N+N+E); //NNE
        out |=  (knight_loc & (~(top)) & (~(right|col(6)))) << (N+E+E); //NEE
        out |=  (knight_loc & (~(bottom)) & (~(right|col(6)))) << (N+E+E); //SEE
        out |=  (knight_loc & (~(bottom | row(1))) & (~(right)) << (N+E+E)); //SSE
        out |=  (knight_loc & (~(bottom | row(1))) & (~(left)) << (N+E+E)); //SSW
        out |=  (knight_loc & (~(bottom)) & (~(left|col(1)))) << (N+E+E); //SWW
        out |=  (knight_loc & (~(top)) & (~(left|col(1)))) << (N+E+E); //NWW
        out |= (knight_loc & (~(top | row(6))) & (~left)) << (N+N+E); //NNW
        return out;
    }
}