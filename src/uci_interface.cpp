#include "uci_interface.h"
#include <cstdlib>
#include <iostream>
#include <movegen_benchmark.h>
#include <string>

void UCIInterface::process_uci_command() {
    UCIInterface::uci_response("id name " + ID_name);
    UCIInterface::uci_response("id author " + ID_author);
    UCIInterface::uci_response("uciok");
}

void UCIInterface::process_isready_command() { UCIInterface::uci_response("readyok"); }
void UCIInterface::process_quit_command() {
    UCIInterface::uci_response("Exiting...");
    exit(0);
}

void UCIInterface::process_new_game_command() { UCIInterface::uci_response("New game started."); }
void UCIInterface::process_go_command(std::string command) {
    // Initialize board:
    if (debug_mode)
        UCIInterface::uci_response("Processing go command: " + command);
    size_t pos = 0;
    while (pos < command.size()) {
        while (pos < command.size() && command[pos] == ' ')
            pos++;

        if (pos >= command.size())
            break;

        size_t start = pos;
        while (pos < command.size() && command[pos] != ' ')
            pos++;

        std::string token = command.substr(start, pos - start);

        if (token == "perft") {
            // Assume is followed by one integer after.
            while (pos < command.size() && command[pos] == ' ')
                pos++;  // Find next int.
            //
            start = pos;
            while (pos < command.size() && command[pos] != ' ')  // Find ending of next int.
                pos++;

            std::string int_token = command.substr(start, pos - start);
            try {
                int depth = std::stoi(int_token);
                int nodes =
                    movegen_benchmark::gen_num_moves(Game::instance().get_state(), depth, 0);
                std::string nodes_searched = std::to_string(nodes);
                UCIInterface::uci_response("\nNodes searched: " + nodes_searched);
            } catch (const ::std::exception &e) {
                UCIInterface::uci_response(
                    "Error: \"perft\" command should be followed by an integer but found: " +
                    int_token);
            }

        } else {
        }
    }
}

void UCIInterface::process_position_command(std::string command) {
    // Example: "position startpos moves e2e4 e7e5"
    // Initialize board:
    Game::instance().set_fen(NotationInterface::starting_FEN());

    size_t pos = 0;
    while (pos < command.size()) {
        while (pos < command.size() && command[pos] == ' ')
            pos++;

        if (pos >= command.size())
            break;

        size_t start = pos;
        while (pos < command.size() && command[pos] != ' ')
            pos++;

        std::string token = command.substr(start, pos - start);
        if (token == "startpos") {
        } else if (token == "moves") {
        } else {
            try {
                Move move = Move(token);
                Game::instance().make_move(move);
                if (debug_mode)
                    UCIInterface::uci_response("Move processed: " +
                                               move.toString());  // Informative
            } catch (const std::exception &e) {
                UCIInterface::uci_response("Error processing token as move: " + token);
                // If we hit an invalid move/token, we should likely stop.
                break;
            }
        }
    }
}

void UCIInterface::process_ponder_command() {
    UCIInterface::uci_response("Processing ponder command.");
}
void UCIInterface::send_bestmove() { UCIInterface::uci_response("bestmove e2e4 ponder e7e5"); }

void UCIInterface::process_board_command() { Game::instance().display_board(); }

void UCIInterface::process_fen_command(std::string command) {
    UCIInterface::uci_response("Processing FEN command: " + command);
    bool success = Game::instance().set_fen(command);
    if (!success)
        UCIInterface::uci_response("Invalid FEN command: " + command);
    UCIInterface::uci_response("FEN command processed.");
}

void UCIInterface::uci_response(std::string response) { std::cout << response << std::endl; }
