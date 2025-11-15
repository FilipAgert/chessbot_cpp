#include <piece.h>
#include <board_state.h>
#include <constants.h>
#include <notation_interface.h>

class Game {
public:
    static Game& instance(){
        static Game game_instance;
        return game_instance;
    }
    void start_game();
    void end_game();
    void reset_game();
    void make_move(uint8_t start_square, uint8_t end_square);
    void undo_move();
    void display_board();
    std::string get_fen() const;
    /**
     * @brief Set the game state from FEN. If not success, game state now undefined.
     * 
     * @param FEN FEN string
     * @return true OK
     * @return false Game state undefined.
     */
    bool set_fen(std::string FEN);
private:
    BoardState state;
    Game() = default;
};
