#include <game.h>
#include <string>




game::game(){
    this->current_turn = 0;
    this->move_count = 0;
    this->ply_count = 0;
    this->castle_rights = 0;
}