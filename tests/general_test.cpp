#include "notation_interface.h"
#include <gtest/gtest.h>

TEST(idx_test, flip_idx) {
    int king_white = NotationInterface::idx_from_string("e1");
    int king_black = NotationInterface::idx_flip(king_white);
    ASSERT_EQ(king_black, NotationInterface::idx_from_string("e8"));
    ASSERT_EQ(NotationInterface::idx_flip(NotationInterface::idx_from_string("b2")), NotationInterface::idx_from_string("b7"));
}