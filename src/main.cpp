// Copyright 2025 Filip Agert
#include "eval/hce/hceval.h"
#include "interface/engine.h"
#include "interface/uci_interface.h"
#include "iostream"
#include "string"
#include "thread"
#include "utils/config.h"
#include "utils/config_parser.h"
#include <queue>
class ThreadSafeCommandQueue {
  private:
    std::queue<std::string> queue_;
    mutable std::mutex mutex_;

  public:
    void push(const std::string &cmd) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(cmd);
    }
    bool try_pop(std::string &cmd) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty())
            return false;
        cmd = std::move(queue_.front());
        queue_.pop();
        return true;
    }
};

int main(int argc, char *argv[]) {
    std::string config_file = "input/input.cfg";
    if (argc > 1) {
        config_file = argv[1];
    }

    ConfigParser parser;
    parser.load(config_file);
    ConfigSettings settings = parser.get_settings();

    Eval::HCE::HCEval eval;
    Engine engine(settings, eval);
    UCIInterface uci(engine);

    static ThreadSafeCommandQueue input_queue;

    // 1. Spawn a background thread whose ONLY job is to block on std::cin
    std::thread input_thread([]() {
        std::string line;
        while (std::getline(std::cin, line)) {
            input_queue.push(line);
            if (line == "quit")
                break;
        }
        input_queue.push("quit");
    });
    input_thread.detach(); // Let it run independently

    std::cout << "Welcome to the UCI interface!" << std::endl;
    std::string input, command, body;
    auto sleeptime = 3ms;
    bool waiting_for_move = false;
    bool running = true;
    while (running) {
        if (input_queue.try_pop(input)) {
            size_t space_pos = input.find(" ");
            if (space_pos == std::string::npos) {
                command = input;
                body = "";
            } else {
                command = input.substr(0, space_pos);
                body = input.substr(space_pos + 1);
            }

            if (command == "uci") {
                uci.process_uci_command(settings);
            } else if (command == "ucinewgame") {
                uci.process_new_game_command();
            } else if (command == "isready") {
                uci.process_isready_command();
            } else if (command == "go") {
                waiting_for_move = true;
                uci.process_go_command(body, true);
            } else if (command == "position") {
                uci.process_position_command(body);
            } else if (command == "bestmove") {
                waiting_for_move = false;
                uci.send_bestmove(false);
            } else if (command == "ponderhit") {
                uci.process_ponderhit_command();
            } else if (command == "d") {
                uci.process_d_command(eval);
            } else if (command == "stop") {
                uci.process_stop_command();
            } else if (command == "bench") {
                uci.process_bench_command(body);
            } else if (command == "quit") {
                uci.process_quit_command();
                std::cout.flush();
                break;
            } else if (command == "setoption") {
                uci.process_setoption_command(body, settings);
                engine.set_settings(settings);
            } else if (command == "self") {
                uci.process_self_command(eval, body);
            } else if (command == "debug") {
                std::cout << "Debug mode is not implemented yet." << std::endl;
            } else if (command == "help") {
                std::cout << "Available commands: uci, isready, go, position, bestmove, ponder, "
                             "newgame, quit, debug, d, self, repeat, help"
                          << std::endl;
            } else {
                std::cout << "Unknown command: " << input << std::endl;
            }
        }
        uci.send_info_if_has();
        if (waiting_for_move && engine.search_finished) {
            uci.send_bestmove(settings.should_ponder);
            waiting_for_move = false;
        }
        std::this_thread::sleep_for(sleeptime);
    }
    return 0;
}
