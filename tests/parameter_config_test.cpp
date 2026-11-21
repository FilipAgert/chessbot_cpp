#include "eval/hce/parameter_config.h"
#include "integer_representation.h"
#include "parameters.h"
#include <gtest/gtest.h>

TEST(Parameter, read_scalars) {
    std::string file = "tests/parameters.json";
    Eval::HCE::ParameterConfig config(file);

    Eval::HCE::EvalParameters p = config.params;

    ASSERT_EQ(p.bishop_double_bonus, 1);
    ASSERT_EQ(p.king_dist, 2);

    ASSERT_EQ(p.movevals[pieces::pawn], 17);
    ASSERT_EQ(p.movevals[pieces::rook], 14);
    ASSERT_EQ(p.piecevals[pieces::rook], 8);

    ASSERT_EQ(p.piecevals[pieces::none], 0);
    ASSERT_EQ(p.movevals[pieces::none], 0);
    ASSERT_EQ(p.passed_dist_to_promotion[0], 0);
    ASSERT_EQ(p.passed_dist_to_promotion[1], 18);
}

TEST(Parameter, read_pst) {
    std::string file = "tests/parameters.json";
    Eval::HCE::ParameterConfig config(file);

    Eval::HCE::EvalParameters p = config.params;

    ASSERT_EQ(p.pst<true>(pieces::king, 0), 0);
    ASSERT_EQ(p.pst<true>(pieces::king, 1), 1);
    ASSERT_EQ(p.pst<true>(pieces::king, 2), 2);
    ASSERT_EQ(p.pst<true>(pieces::king, 9), 4);
}