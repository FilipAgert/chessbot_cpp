#include "uci_interface.h"
#include <chrono>
#include <config.h>
#include <cstdlib>
#include <eval.h>
#include <exceptions.h>
#include <iostream>
#include <movegen_benchmark.h>
#include <sstream>
#include <string>
#include <time_manager.h>

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
void UCIInterface::send_info_msg(InfoMsg msg) {
    std::string str_depth = "depth " + std::to_string(msg.depth);
    std::string str_seldepth = "seldepth " + std::to_string(msg.seldepth);
    std::string str_time = "time " + std::to_string(msg.time);
    std::string str_nodes = "nodes " + std::to_string(msg.nodes);
    std::string str_nmoves = "nmoves " + std::to_string(msg.moves_generated);
    std::string str_nps = "knps ";
    if (msg.time > 0) {
        str_nps.append(std::to_string((msg.nodes) / msg.time));
    } else {
        str_nps.append("NaN");
    }
    std::string str_pv = "pv";
    for (Move m : msg.pv)
        str_pv.append(m.toString() + " ");

    std::string str_currmove = "currmove " + msg.currmove.toString();
    std::string str_score = "score " + std::to_string(msg.score);
    std::string str_d0score = "d0 " + std::to_string(msg.d0score);
    std::string str_mate = "mate " + std::to_string(msg.mate);

    std::string str_infostub = "info";
    std::string str =
        join({str_infostub, str_depth, str_pv, str_score, str_time, str_nodes, str_nps}, ' ');
    UCIInterface::uci_response(str);
}
void UCIInterface::send_info_if_has() {
    for (; !Game::instance().info_queue.empty(); Game::instance().info_queue.pop()) {
        InfoMsg info = Game::instance().info_queue.front();
        UCIInterface::send_info_msg(info);
    }
}
void UCIInterface::process_go_command(std::string command) {
    // Initialize board:
    if (debug_mode)
        UCIInterface::uci_response("Processing go command: " + command);
    int wtime, btime, winc, binc;
    wtime = btime = STANDARD_TIME;
    winc = binc = STANDARD_TINC;
    auto parts = split(command, ' ');
    if (parts.size() > 0) {
        if (parts[0] == "perft") {
            if (parts.size() < 2) {
                UCIInterface::uci_response(
                    "A perft command must have the structure go perft <depth>");
                return;
            }
            std::string int_token = parts[1];
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
            return;
        } else {

            // Process wtime <> btime <> winc <> binc<>
            // Alterantively infinite
            size_t idx = 0;
            while (idx < parts.size()) {
                std::string token = parts[idx];
                std::optional<int> oint;
                if (token == "wtime") {
                    oint = try_process_int(parts[idx + 1]);
                    if (oint) {
                        wtime = oint.value();
                        idx++;
                    }
                } else if (token == "btime") {
                    oint = try_process_int(parts[idx + 1]);
                    if (oint) {
                        btime = oint.value();
                        idx++;
                    }

                } else if (token == "binc") {
                    oint = try_process_int(parts[idx + 1]);
                    if (oint) {
                        binc = oint.value();
                        idx++;
                    }
                } else if (token == "winc") {
                    oint = try_process_int(parts[idx + 1]);
                    if (oint) {
                        winc = oint.value();
                        idx++;
                    }
                } else if (token == "infinite") {
                    NotImplemented(
                        "Infinite time control is not implemented yet");  // TODO: Implement.
                }
                idx++;
            }
        }
    }
    time_control rem_time =
        time_control({.wtime = wtime, .btime = btime, .winc = winc, .binc = binc});
    Game::instance().start_thinking(rem_time);  // Enter ponder loop
    UCIInterface::send_info_if_has();
    UCIInterface::send_bestmove();
}

std::optional<int> UCIInterface::try_process_int(std::string intstring) {
    int depth;
    try {
        depth = std::stoi(intstring);
    } catch (const std::exception &e) {
        UCIInterface::uci_response("Error processing string to int: " + intstring);
        return {};
    }
    return std::make_optional(depth);
}
void UCIInterface::process_position_command(std::string command) {
    // Example: "position startpos moves e2e4 e7e5"
    // Initialize board:

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
            Game::instance().set_fen(NotationInterface::starting_FEN());
        } else if (token == "moves") {
        } else if (token == "fen") {
            while (pos < command.size() && command[pos] != ' ')  // Start of fen
                pos++;
            start = pos + 1;
            while (pos < command.size() && command[pos] == ' ')  // end of fen.
                pos++;
            while (pos < command.size() && command[pos] != ' ')  // start col
                pos++;
            while (pos < command.size() && command[pos] == ' ')  // end col
                pos++;
            while (pos < command.size() && command[pos] != ' ')  // start cast
                pos++;
            while (pos < command.size() && command[pos] == ' ')  // end cast
                pos++;
            while (pos < command.size() && command[pos] != ' ')  // start ply
                pos++;
            while (pos < command.size() && command[pos] == ' ')  // end ply
                pos++;
            while (pos < command.size() && command[pos] != ' ')  // start move
                pos++;
            while (pos < command.size() && command[pos] == ' ')  // end move
                pos++;
            while (pos < command.size() && command[pos] != ' ')  // start move
                pos++;
            while (pos < command.size() && command[pos] == ' ')  // end move
                pos++;
            while (pos < command.size() && command[pos] != ' ')  // start move
                pos++;
            UCIInterface::process_fen_command(command.substr(start, pos - start + 1));
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
void UCIInterface::send_bestmove() {
    Move bestmove = Game::instance().get_bestmove();
    UCIInterface::uci_response("bestmove " + bestmove.toString());
    // TODO: handle if there is no best move (mate).
}

void UCIInterface::process_d_command() {
    Game::instance().display_board();
    UCIInterface::uci_response(Game::instance().get_fen());
    int eval = EvalState::eval(Game::instance().get_state());
    UCIInterface::uci_response("Board evaluation (0 depth): " + std::to_string(eval));
}
void UCIInterface::process_bench_command(std::string command) {
    // Should be structured like:
    // <fentype> <depth> <threads>
    std::vector<std::string> parts = UCIInterface::split(command, ' ');
    int depthloc = 1;
    if (parts.size() == 8) {
        std::vector<std::string> fenparts = parts;
        int fenl = 6;
        for (int i = 0; i < fenl; i++)
            fenparts[i] = parts[i];
        depthloc = fenl;
        std::string fen = UCIInterface::join(fenparts, ' ');
        Game::instance().set_fen(fen);
    } else if (parts.size() == 3) {
        if (parts[0] == "current") {
        } else if (parts[0] == "default") {
            Game::instance().reset_game();
        } else {
            UCIInterface::uci_response(
                "fentype must be one of: current, default, or a literal <fen> string ");
            return;
        }
    } else {
        UCIInterface::uci_response(
            "Invalid bench command structure. Must be <fentype> <depth> <threads>.");
        return;
    }

    int depth = std::stoi(parts[depthloc]);
    int threads = std::stoi(parts[depthloc + 1]);
    UCIInterface::uci_response("Generating moves to depth: " + std::to_string(depth));
    auto start = std::chrono::high_resolution_clock::now();
    int64_t nummoves = movegen_benchmark::gen_num_moves(Game::instance().get_state(), depth, -1);
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    UCIInterface::uci_response(std::to_string(nummoves) + " nodes found at this depth.");
    UCIInterface::uci_response("Time taken: " + std::to_string(duration.count()) + " ms.");
    int64_t mps = (nummoves / duration.count());
    UCIInterface::uci_response("1000 Nodes per second: " + std::to_string(mps));
}

void UCIInterface::process_fen_command(std::string command) {
    UCIInterface::uci_response("Processing FEN command: " + command);
    bool success = Game::instance().set_fen(command);
    if (!success)
        UCIInterface::uci_response("Invalid FEN command: " + command);
    UCIInterface::uci_response("FEN command processed.");
}

void UCIInterface::uci_response(std::string response) { std::cout << response << std::endl; }
std::vector<std::string> UCIInterface::split(std::string full, char del) {
    std::stringstream ss(full);
    std::string temp;
    std::vector<std::string> out;
    while (std::getline(ss, temp, del)) {
        out.push_back(temp);
    }
    return out;
}
std::string UCIInterface::trim(std::string full) {
    std::string out = "";
    int i;
    for (i = 0; i < full.length() && full[i] == ' '; i++) {
    }
    int start = i;

    for (i = full.length() - 1; i >= 0 && full[i] == ' '; i--) {
    }
    int stop = i;
    if (start <= stop)
        out = full.substr(start, stop - start + 1);
    return out;
}
std::string UCIInterface::join(std::vector<std::string> strings, char del) {
    std::string str = "";
    for (int i = 0; i < strings.size(); i++) {
        if (i != 0)
            str.push_back(del);
        str.append(strings[i]);
    }
    return str;
}
