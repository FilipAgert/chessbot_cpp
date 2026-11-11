#include "eval.h"
#include "str.h"
#include "texel/core.h"
#include "texel/eigen_defs.h"
#include "texel/eval_pos.h"
#include "texel/gradient.h"
#include "texel/input_reader.h"
#include "texel/minimize.h"
#include "utils/str.h"
#include <iostream>
#include <omp.h>
#include <optional>
using namespace Texel;
double round_if_int(double a, bool is_float) {
    if (is_float)
        return std::round(a);
    return a;
};
int main(int argc, char *argv[]) {
    std::cout << "Welcome to texel tuning!\n\n";
    std::cout << "Here we will tune the parameters of the eval function using a dataset of games.\n";
    std::cout << "The goal is to extract positions from labeled high elo games, and using the eval function, predict the winner.\n\n";
    std::cout << "There are generally two ways to do this: Quiesence tuning and standing pat tuning.\n";
    std::cout << "Quiesence tuning will reduce horizon effect but will take longer to run.\n";

    if (argc < 2) {
        std::cerr << "Error: Missing dataset path argument.\n";
        std::cerr << "Usage: " << argv[0] << " <path/to/games.dat>\n";
        return 1;
    }

    std::string path = argv[1];
    std::optional<int> num_lines = std::nullopt;
    if (argc >= 3) {
        num_lines = Utils::try_process_int(argv[2]);
    }

    std::vector<Position> positions = read_dataset(path, num_lines);
    std::cout << "Loaded " << positions.size() << " positions" << std::endl;
#pragma omp parallel
    {
#pragma omp single
        std::cout << "Active threads: " << omp_get_num_threads() << std::endl;
    }

    std::vector<Result> res;
    std::vector<std::string> fens;
    res.reserve(positions.size());
    fens.reserve(positions.size());

    for (size_t i = 0; i < positions.size(); i++) {
        Board test;
        bool success = test.read_fen(positions.at(i).fen);

        if (success) {
            res.push_back((positions.at(i).res));
            fens.push_back((positions.at(i).fen));
        }
    }
    std::cout << "Succesfully processed " << fens.size() << " positions" << std::endl;
    dvec results(res.size());
    for (size_t i = 0; i < res.size(); i++) {
        results(i) = to_win(res[i]);
    }

    // parameter handling
    int num_params = 5 + 6 + 5;
    std::vector<bool> is_float(num_params, false);

    auto create_params = [is_float](const dvec &x) -> EvalParameters {
        dvec rounded(x.size());
        for (size_t i = 0; i < x.size(); i++) {
            rounded(i) = round_if_int(x(i), is_float.at(i));
        }
        std::array<int, 5> piecevals;
        std::array<int, 6> movevals;

        int taken = 0;
        for (size_t i = 0; i < piecevals.size(); i++) {
            piecevals[i] = static_cast<int>(rounded(taken++));
        }
        for (size_t i = 0; i < movevals.size(); i++) {
            movevals[i] = static_cast<int>(rounded(taken++));
        }
        int bdb = rounded(taken++);
        int k2centre = rounded(taken++);
        int passed = rounded(taken++);
        int doubled = rounded(taken++);
        int solo = rounded(taken++);

        return EvalParameters{std::move(piecevals), std::move(movevals), bdb, k2centre, passed, doubled, solo};
    };
    using namespace DefaultVals;
    dvec x0 = (dvec(num_params) << queen, rook, knight, bishop, pawn, king_moveval, queen_moveval, rook_moveval, knight_moveval, bishop_moveval, pawn_moveval,
               bishop_double_bonus, king_dist2centre_value, passed_pawn_eval, doubled_pawn_punishment, solo_pawn_punishment)
                  .finished();

    // create evaluator
    // const EvalParameters params0 = create_params(x0);
    // std::function<double(const dvec &)> eval_k_loss = [create_params, fens, results, params0](const dvec &x) {
    //     double K = x(0);
    //     dvec predictions = eval_positions(params0, fens, K);
    //     return loss(results, predictions);
    // };
    // std::vector<bool> is_k_float(true, 1);
    // MixedGradient K_eval(is_k_float, eval_k_loss);
    double K = 0.63;
    // std::cout << "Minimizing K at start of loop. K0 = " << K << std::endl;
    // dvec xk0 = (dvec(1) << K).finished();
    // MinimizeResult k_res = minimize(xk0, K_eval);
    // std::cout << "Found optimal K: " << k_res.x << std::endl;
    // K = k_res.x(0);

    std::function<double(const dvec &)> eval_param_loss = [create_params, fens, results, K](const dvec &x) {
        const EvalParameters params = create_params(x);
        dvec predictions = eval_positions(params, fens, K);
        return loss(results, predictions);
    };

    std::cout << "\n\nNow minimizing parameters" << std::endl;
    MixedGradient param_eval(is_float, eval_param_loss);
    auto print_fnc = [create_params](dvec x) { return create_params(x).to_string(); };
    MinimizeResult param_res = minimize(x0, param_eval, print_fnc);
    std::cout << "Parameters found\n" << param_res.x;
    return 0;
}