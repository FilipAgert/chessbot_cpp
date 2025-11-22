#include <bitboard.h>
using namespace dirs;
using namespace masks;
uint64_t pawn_forward_moves(const uint64_t pawn_bb, const uint64_t all_bb, const uint8_t pawn_col){
    int dir = (pawn_col == pieces::white)*N + (pawn_col==pieces::black)*S;//branchless assignment
    uint8_t rowi = 1*(pawn_col == pieces::white) + 6*(pawn_col == pieces::black); //Branchless execution. Evaluates to 1 or 7 depending on color.
    uint64_t blocker = BitBoard::shift_bb(all_bb, -dir); //Blocking pieces are one up
    uint64_t moves = BitBoard::shift_bb(pawn_bb & ~blocker, dir); //All one moves up
    blocker = blocker | BitBoard::shift_bb(blocker, -dir); //Pieces now block two moves...
    moves |= BitBoard::shift_bb(pawn_bb & ~blocker & row(rowi), dir, 2);
    return moves;
}
uint64_t pawn_attack_moves(const uint64_t pawn_bb, const uint64_t enemy_bb, const uint64_t ep_bb, const uint8_t pawn_col){
    int dir = (pawn_col == pieces::white)*N + (pawn_col==pieces::black)*S;//branchless assignment
    uint64_t moves = BitBoard::shift_bb(pawn_bb & ~col(7), dir+1) | BitBoard::shift_bb(pawn_bb & ~col(0), dir-1);
    //dir + 1 is either NE or SE. Cannot go to the right if we are in the rightmost column. Likewise for dir -1 with left column.
    moves &= (enemy_bb | ep_bb); //Require either enemy there or en_passant there. 
    return moves;
}
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
        uint64_t hit = ray(king_bb, N, 0,1);
        hit |= ray(king_bb, NE, 0,1);
        hit |= ray(king_bb, E, 0,1);
        hit |= ray(king_bb, SE,0, 1);
        hit |= ray(king_bb, S, 0,1);
        hit |= ray(king_bb, SW,0, 1);
        hit |= ray(king_bb, W, 0,1);
        hit |= ray(king_bb, NW,0, 1);
        return hit & ~friendly_bb;
    }
    uint64_t pawn_moves(const uint64_t pawn_bb, const uint64_t friendly_bb, const uint64_t enemy_bb, const uint64_t ep_bb, const uint8_t pawn_col){
        return pawn_forward_moves(pawn_bb, friendly_bb | enemy_bb, pawn_col) | pawn_attack_moves(pawn_bb, enemy_bb, ep_bb, pawn_col);
    }

    std::string to_string_bb(uint64_t bb) {  
        std::stringstream ss;
        
        // File labels
        ss << "\n    a b c d e f g h\n";
        ss << "   -----------------\n";

        // Iterate ranks from 8 (index 7) down to 1 (index 0)
        for (int rank = 7; rank >= 0; --rank) {
            ss << rank + 1 << " |"; // Rank label
            for (int file = 0; file < 8; ++file) {
                int index = rank * 8 + file;
                // Check if the bit at the current square index is set
                if ((bb >> index) & 1) {
                    ss << " 1"; // High bit
                } else {
                    ss << " -"; // Low bit
                }
            }
            ss << " |\n";
        }
        ss << "   -----------------\n";
        
        return ss.str();
    };
    
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