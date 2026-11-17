// Copyright 2025 Filip Agert
#ifndef UCI_INTERFACE_H
#define UCI_INTERFACE_H
#include "eval/eval.h"
#include "search/game.h"
#include "utils/config.h"

#include <string>
class UCIInterface {
  public:
    void process_uci_command(ConfigSettings settings);
    void uci_response(std::string response);
    void process_isready_command();
    void process_quit_command();
    void process_new_game_command();
    void send_bestmove(bool pondering);
    void process_d_command(Eval::Eval &eval);
    void process_ponderhit_command();
    void process_self_command(Eval::Eval &eval, std::string command);

    /**
     * @brief Checks if game has info to send, if so: send info. Empties game queue.
     *
     */
    void send_info_if_has();
    /**
     * @brief Sends info message to console
     *
     * @param[[TODO:direction]] msg [TODO:description]
     */
    void send_info_msg(InfoMsg msg);
    /**
     * @brief The go command has the following functionality:
     * "go perft <depth>": gets number of nodes at a certain depth.
     * "go eval": Evaluates current board state with eval function.
     * "go": Gets bestmove and plays it.
     *
     * @param[[TODO:direction]] command [TODO:description]
     */
    void process_go_command(std::string command, bool assign_worker);
    void process_position_command(std::string command);
    void process_fen_command(std::string command);
    void process_stop_command();
    void process_setoption_command(std::string command, ConfigSettings &settings);
    /**
     * @brief Enters benchmarking mode. This evaluates to a certain depth all possible moves.
     * @param[in] command: String with three parts: <fentype> <depth> <threads>
     * Where fentype is one of: current, default or a literal fen string
     * depth is number of ply moves to search
     * threads (unused) is number of threads to search with.
     */
    void process_bench_command(std::string command);

    UCIInterface(Game &game) : game(game){};

  private:
    Game &game;
};
#endif
