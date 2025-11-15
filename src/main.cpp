#include "iostream"
#include "string"
#include "uci_interface.h"

int main() {
    std::string input;
    std::cout << "Welcome to the UCI interface!" << std::endl;
    std::string command, body;
    

    do {
        std::getline(std::cin, input);
        size_t space_pos = input.find(" ");
        if(space_pos == std::string::npos){
            command = input;
            body = "";
        } else {
            command = input.substr(0, space_pos);
            body = input.substr(space_pos + 1);
        }

        if (command == "uci") {
            UCIInterface::process_uci_command();
        } else if(command == "isready"){
            UCIInterface::process_isready_command();
        } else if(command == "go"){
            UCIInterface::process_go_command(body);
        } else if(command == "position"){
            UCIInterface::process_position_command(body);
        } else if (command == "fen"){
           UCIInterface::process_fen_command(body);
        } else if(command == "bestmove"){
            UCIInterface::send_bestmove();
        } else if(command == "ponder"){
            UCIInterface::process_ponder_command();
        } else if (command == "newgame") {
            UCIInterface::process_new_game_command();
        } else if (command == "quit") {
            UCIInterface::process_quit_command();
        } else if (command == "debug") {
            std::cout << "Debug mode is not implemented yet." << std::endl;
        } else if (command == "help") {
            std::cout << "Available commands: uci, isready, go, position, fen, bestmove, ponder, newgame, stop, debug, help" << std::endl;
        } else {
            std::cout << "Unknown command: " << input << std::endl;
        }
    }while (true);
    return 0;
}



