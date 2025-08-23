#include "iostream"
#include "string"
#include "uci_interface.h"

int main() {
    std::string input;
    std::cout << "Welcome to the UCI interface!" << std::endl;

    

    do {
        std::getline(std::cin, input);

        if (input == "uci") {
            UCIInterface::process_uci_command();
        } else if(input == "isready"){
            UCIInterface::process_isready_command();
        } else if(input == "go"){
            UCIInterface::process_go_command(input);
        } else if(input == "position"){
            UCIInterface::process_position_command(input);
        } else if(input == "bestmove"){
            UCIInterface::send_bestmove();
        } else if(input == "ponder"){
            UCIInterface::process_ponder_command();
        } else if (input == "newgame") {
            UCIInterface::process_new_game_command();
        } else if (input == "quit") {
            UCIInterface::process_quit_command();
        } else if (input == "debug") {
            std::cout << "Debug mode is not implemented yet." << std::endl;
        } else if (input == "help") {
            std::cout << "Available commands: uci, isready, go, position, bestmove, ponder, newgame, stop, debug, help" << std::endl;
        } else {
            std::cout << "Unknown command: " << input << std::endl;
        }
    }while (true);
    return 0;
}



