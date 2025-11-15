#include "uci_interface.h"
#include <cstdlib>


void UCIInterface::process_uci_command(){
    UCIInterface::uci_response("id name " + ID_name);
    UCIInterface::uci_response("id author " + ID_author);
    UCIInterface::uci_response("uciok");
}

void UCIInterface::process_isready_command() {
    UCIInterface::uci_response("readyok");
}
void UCIInterface::process_quit_command() {
    UCIInterface::uci_response("Exiting...");
    exit(0);
}

void UCIInterface::process_new_game_command() {
    UCIInterface::uci_response("New game started.");
}
void UCIInterface::process_go_command(std::string command) {
    UCIInterface::uci_response("Processing go command: " + command);
}
void UCIInterface::process_position_command(std::string command) {
    // Example: "position startpos moves e2e4 e7e5"
    UCIInterface::uci_response("Processing position command: " + command);
}

void UCIInterface::process_ponder_command() {
    UCIInterface::uci_response("Processing ponder command.");
}
void UCIInterface::send_bestmove() {
    UCIInterface::uci_response("bestmove e2e4 ponder e7e5");
}

void UCIInterface::process_fen_command(std::string command){
    UCIInterface::uci_response("Processing FEN command: " + command);
    bool success = Game::instance().set_fen(command);
    if(!success) UCIInterface::uci_response("Invalid FEN command: " + command);
    UCIInterface::uci_response("FEN command processed.");
}


void UCIInterface::uci_response(std::string response) {
    std::cout << response << std::endl;
}