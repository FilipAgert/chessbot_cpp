#include "eval.h"
#include "integer_representation.h"
#include "str.h"
#include "texel/core.h"
#include "texel/eigen_defs.h"
#include "texel/eval_pos.h"
#include "texel/gradient.h"
#include "texel/input_reader.h"
#include "texel/minimize.h"
#include "utils/str.h"

#include <iostream>
#include <optional>

#ifdef _OPENMP
#include <omp.h>
#else
inline int omp_get_num_threads() { return 1; }
inline int omp_get_thread_num() { return 0; }
inline int omp_get_max_threads() { return 1; }
#endif

using namespace Texel;

double round_if_int(double a, bool is_float) {
    if (is_float)
        return std::round(a);
    return a;
}

int main(int argc, char *argv[]) {
    std::cout << "Welcome to texel tuning!\n\n";
    std::cout << "Here we will tune the parameters of the eval function using a dataset of games.\n";
    std::cout << "The goal is to extract positions from labeled high elo games, and using the eval function, predict the winner.\n\n";
    std::cout << "There are generally two ways to do this: Quiesence tuning and standing pat tuning.\n";
    std::cout << "Quiesence tuning will reduce horizon effect but will take longer to run.\n";

    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <path/to/games.dat> <verification_positions> [max_lines]\n";
        return 1;
    }

    std::string path = argv[1];

    auto verification_arg = Utils::try_process_int(argv[2]);
    if (!verification_arg || *verification_arg < 0) {
        std::cerr << "Invalid verification position count.\n";
        return 1;
    }

    const size_t verification_count = static_cast<size_t>(*verification_arg);

    std::optional<int> num_lines = std::nullopt;
    if (argc >= 4)
        num_lines = Utils::try_process_int(argv[3]);

    // Load dataset
    std::vector<Position> positions = read_dataset(path, num_lines);

    std::cout << "Loaded " << positions.size() << " positions" << std::endl;

#pragma omp parallel
    {
#pragma omp single
        std::cout << "Active threads: " << omp_get_num_threads() << std::endl;
    }

    // Validate and extract positions
    std::vector<Result> res;
    std::vector<std::string> fens;

    res.reserve(positions.size());
    fens.reserve(positions.size());

    for (const auto &position : positions) {
        Board test;

        if (test.read_fen(position.fen)) {
            res.push_back(position.res);
            fens.push_back(position.fen);
        }
    }

    std::cout << "Successfully processed " << fens.size() << " positions" << std::endl;

    // Split training and verification data
    if (verification_count >= fens.size()) {
        std::cerr << "Error: Verification set must be smaller than the dataset.\n";
        return 1;
    }

    const size_t training_count = fens.size() - verification_count;

    std::vector<std::string> training_fens(fens.begin(), fens.begin() + training_count);

    std::vector<Result> training_res(res.begin(), res.begin() + training_count);

    std::vector<std::string> verification_fens(fens.begin() + training_count, fens.end());

    std::vector<Result> verification_res(res.begin() + training_count, res.end());

    std::cout << "\nDataset split:" << "\n  Training:    " << training_count << "\n  Verification: " << verification_count << std::endl;

    // Convert training results to win probabilities
    dvec training_results(training_res.size());

    for (size_t i = 0; i < training_res.size(); i++)
        training_results(i) = to_win(training_res[i]);

    // Convert verification results to win probabilities
    dvec verification_results(verification_res.size());

    for (size_t i = 0; i < verification_res.size(); i++)
        verification_results(i) = to_win(verification_res[i]);

    // Initial parameters
    EvalParameters p0;

    int num_params = p0.num_params();
    std::vector<bool> is_float(num_params, false);

    auto create_params = [is_float, p0](const dvec &x) -> EvalParameters {
        std::vector<int> rounded(x.size());

        for (size_t i = 0; i < x.size(); i++)
            rounded[i] = round_if_int(x(i), is_float.at(i));

        EvalParameters p = p0;
        p.from_vector(rounded);

        return p;
    };

    dvec x0(num_params);

    std::vector<int> params = p0.to_vector();

    for (size_t i = 0; i < num_params; i++)
        x0(i) = params[i];

    // Evaluation constant
    double K = 0.63;

    // Training objective: verification data is never used here.
    std::function<double(const dvec &)> eval_param_loss = [create_params, training_fens, training_results, K](const dvec &x) {
        const EvalParameters params = create_params(x);

        dvec predictions = eval_positions(params, training_fens, K);

        return loss(training_results, predictions);
    };

    // Measure initial verification loss
    dvec verification_predictions_before = eval_positions(p0, verification_fens, K);

    double verification_loss_before = loss(verification_results, verification_predictions_before);

    std::cout << "\n\nNow minimizing parameters" << std::endl;
    std::cout << "Initial parameters:\n" << p0.to_string() << std::endl;

    MixedGradient param_eval(is_float, eval_param_loss);

    auto print_fnc = [create_params](dvec x) { return create_params(x).to_string(); };

    // Minimize using training data only
    MinimizeResult param_res = minimize(x0, param_eval, print_fnc);

    EvalParameters optimized_params = create_params(param_res.x);

    // Measure final verification loss
    dvec verification_predictions_after = eval_positions(optimized_params, verification_fens, K);

    double verification_loss_after = loss(verification_results, verification_predictions_after);

    // Results
    std::cout << "\n\n========== RESULTS ==========\n";

    std::cout << "\nOptimized parameters:\n" << optimized_params.to_string() << std::endl;

    std::cout << "\nVerification loss:\n";
    std::cout << "  Before: " << verification_loss_before << std::endl;
    std::cout << "  After:  " << verification_loss_after << std::endl;

    std::cout << "  Change: " << verification_loss_after - verification_loss_before << std::endl;

    if (verification_loss_before != 0.0) {
        double improvement = 100.0 * (verification_loss_before - verification_loss_after) / verification_loss_before;

        std::cout << "  Improvement: " << improvement << "%" << std::endl;
    }

    std::cout << "\n=============================\n";

    return 0;
}