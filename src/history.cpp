#include <history.h>


History::History(){
    reset_history();
}

void History::reset_history() {
    white_history.fill(EMPTY);
    black_history.fill(EMPTY);
}