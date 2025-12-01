// Copyright 2025 Filip Agert
#ifndef UCI_INTERFACE_H
#define UCI_INTERFACE_H
#include <game.h>

#include <string>
#include <vector>
class UCIInterface {
 public:
    static void process_uci_command();
    static void uci_response(std::string response);
    static void process_isready_command();
    static void process_quit_command();
    static void process_new_game_command();
    static void send_bestmove();
    static void process_d_command();
    static void process_ponder_command();
    static void process_go_command(std::string command);
    static void process_position_command(std::string command);
    static void process_fen_command(std::string command);
    /**
     * @brief Enters benchmarking mode. This evaluates to a certain depth all possible moves.
     * @param[in] command: String with three parts: <fentype> <depth> <threads>
     * Where fentype is one of: current, default or a literal fen string
     * depth is number of ply moves to search
     * threads (unused) is number of threads to search with.
     */
    static void process_bench_command(std::string command);

 private:
    UCIInterface() = delete;

    /**
     * @brief Split string into its component parts.
     *
     * @param[[TODO:direction]] full [TODO:description]
     * @param[[TODO:direction]] del [TODO:description]
     */
    std::vector<std::string> split(std::string full, char del);
    /**
     * @brief Trim whitespaces from a string
     *
     * @param[[TODO:direction]] full [TODO:description]
     */
    std::string trim(std::string full);
};
#endif
