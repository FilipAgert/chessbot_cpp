#include <game.h>
#include <string>



game :: set_fen(std::string fen){
    this->game_board.clear_board();
    
}

game::game(){
    this->current_turn = 0;
    this->move_count = 0;
    this->ply_count = 0;
    this->castle_rights = 0;
    this->game_board = board();
}