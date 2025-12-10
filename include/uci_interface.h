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
    static void process_self_command(std::string command);

    /**
     * @brief Checks if game has info to send, if so: send info. Empties game queue.
     *
     */
    static void send_info_if_has();
    /**
     * @brief Sends info message to console
     *
     * @param[[TODO:direction]] msg [TODO:description]
     */
    static void send_info_msg(InfoMsg msg);
    /**
     * @brief The go command has the following functionality:
     * "go perft <depth>": gets number of nodes at a certain depth.
     * "go eval": Evaluates current board state with eval function.
     * "go": Gets bestmove and plays it.
     *
     * @param[[TODO:direction]] command [TODO:description]
     */
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
    static std::vector<std::string> split(std::string full, char del);
    /**
     * @brief Trim whitespaces from a string
     *
     * @param[[TODO:direction]] full [TODO:description]
     */
    static std::string trim(std::string full);
    /**
     * @brief Join together vector of strings separated by delimiter.
     *
     * @param[[TODO:direction]] strings [TODO:description]
     * @param[[TODO:direction]] del [TODO:description]
     */
    static std::string join(std::vector<std::string> strings, char del);
    static std::optional<int> try_process_int(std::string intstring);
};
#endif
