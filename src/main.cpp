// Copyright 2025 Filip Agert
#include "iostream"
#include "string"
#include "thread"
#include "uci_interface.h"
#include "config_parser.h"
#include "config.h"
#include "game.h"
class ThreadSafeCommandQueue {
private:
    std::queue<std::string> queue_;
    mutable std::mutex mutex_;
public:
    void push(const std::string& cmd) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(cmd);
    }
    bool try_pop(std::string& cmd) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) return false;
        cmd = std::move(queue_.front());
        queue_.pop();
        return true;
    }
};

int main(int argc, char*argv[]) {
    std::string config_file = "input/input.cfg";
    if (argc > 1){
        config_file = argv[1];
    }
    
    ConfigParser parser;
    parser.load(config_file);
    ConfigSettings settings = parser.get_settings();
    Game::instance().set_settings(settings);
    
    ThreadSafeCommandQueue input_queue;

    // 1. Spawn a background thread whose ONLY job is to block on std::cin
    std::thread input_thread([&input_queue]() {
        std::string line;
        while (std::getline(std::cin, line)) {
            input_queue.push(line);
        }
    });
    input_thread.detach(); // Let it run independently


    std::cout << "Welcome to the UCI interface!" << std::endl;
    std::string input, command, body;
    auto sleeptime = 3ms;
    bool waiting_for_move = false;
    bool running=true;
    while(running) {
        if(input_queue.try_pop(input)){
            size_t space_pos = input.find(" ");
            if (space_pos == std::string::npos) {
                command = input;
                body = "";
            } else {
                command = input.substr(0, space_pos);
                body = input.substr(space_pos + 1);
            }

            if (command == "uci") {
                UCIInterface::process_uci_command();
            } else if (command == "isready") {
                UCIInterface::process_isready_command();
            } else if (command == "go") {
                waiting_for_move = true;
                UCIInterface::process_go_command(body);
            } else if (command == "position") {
                UCIInterface::process_position_command(body);
            } else if (command == "bestmove") {
                waiting_for_move = false;
                UCIInterface::send_bestmove();
            } else if (command == "ponder") {
                UCIInterface::process_ponder_command();
            } else if (command == "newgame") {
                UCIInterface::process_new_game_command();
            } else if (command == "d") {
                UCIInterface::process_d_command();
            } else if (command == "stop"){
                UCIInterface::process_stop_command();
            } else if (command == "bench") {
                UCIInterface::process_bench_command(body);
            } else if (command == "quit") {
                running=false;
                UCIInterface::process_quit_command();
            } else if (command == "self") {
                UCIInterface::process_self_command(body);
            } else if (command == "repeat") {
                UCIInterface::process_repeat_command();
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
        UCIInterface::send_info_if_has();
        if (waiting_for_move && Game::instance().search_finished){
            UCIInterface::send_bestmove();
            waiting_for_move=false;
        }
        std::this_thread::sleep_for(sleeptime);
    }
    return 0;
}
