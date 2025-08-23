#ifndef UCI_INTERFACE_H
#define UCI_INTERFACE_H

#include <iostream>
#include <string>
#include "constants.h"

class UCIInterface {
public:
    static void process_uci_command();
    static void uci_response(std::string response); 
    static void process_isready_command();
    static void process_quit_command();
    static void process_new_game_command();
    static void send_bestmove();
    static void process_ponder_command();
    static void process_go_command(std::string command);
    static void process_position_command(std::string command);
private:
    UCIInterface() = delete;
};
#endif