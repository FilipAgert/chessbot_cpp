#include "config.h"
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
#include <argparse/argparse.hpp>
#include <iostream>
#include <optional>
#include <stdexcept>

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

    argparse::ArgumentParser parser(ID_name, ID_version);
    parser.add_argument("file").help("File containing <fen>, <result>, <eval>");
    parser.add_argument("--objective").help("Objective in [result, eval]. Train on game result or stockfish eval.").default_value("result");
    parser.add_argument("--nload").help("Number of positions to load.").scan<'i', int>();
    parser.add_argument("--nvalidation").help("Number of positions to validate on. Subset of nload.").scan<'i', int>();
    parser.add_argument("--method").help("Method to compute evaluation. [direct, quiesence]. Direct is faster but less accurate.").default_value("direct");
    try {
        parser.parse_args(argc, argv);
    } catch (const std::exception &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        std::exit(1);
    }

    std::string path = parser.get<std::string>("file");
    int nload = parser.get<int>("--nload");
    int nvalidation = parser.get<int>("--nvalidation");
    std::string objective = parser.get<std::string>("--objective");
    std::string method = parser.get<std::string>("--method");

    // Load dataset
    std::vector<Position> positions = read_dataset(path, {nload});

    std::cout << "Loaded " << positions.size() << " positions" << std::endl;

#pragma omp parallel
    {
#pragma omp single
        std::cout << "Active threads: " << omp_get_num_threads() << std::endl;
    }

    // Validate and extract positions
    std::vector<std::string> fens;
    fens.reserve(positions.size());
    for (const auto &position : positions) {
        Board test;
        if (test.read_fen(position.fen)) {
            fens.push_back(position.fen);
        }
    }

    std::cout << "Successfully processed " << fens.size() << " positions" << std::endl;

    // Split training and verification data
    if (nvalidation >= fens.size()) {
        std::cerr << "Error: Verification set must be smaller than the dataset.\n";
        return 1;
    }

    const size_t training_count = fens.size() - nvalidation;
    std::vector<Position> training_res(positions.begin(), positions.begin() + training_count);
    std::vector<Position> validation_res(positions.begin() + training_count, positions.end());

    std::vector<std::string> training_fens(fens.begin(), fens.begin() + training_count);
    std::vector<std::string> verification_fens(fens.begin() + training_count, fens.end());

    std::cout << "\nDataset split:" << "\n  Training:    " << training_count << "\n  Verification: " << nvalidation << std::endl;
    // Evaluation constant
    double K = 0.63;
    auto to_res = [K, objective](std::vector<Position> &res) {
        dvec out(res.size());
        for (size_t i = 0; i < res.size(); i++) {
            if (objective == "result")
                out(i) = to_win(res[i].res);
            else {
                if (!res[i].eval)
                    throw std::runtime_error("Must have eval if using mode <eval>");
                out(i) = sigmoid(res[i].eval.value(), K);
            }
        }
        return out;
    };
    dvec training_results = to_res(training_res);
    dvec validation_results = to_res(validation_res);

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

    // Training objective: verification data is never used here.
    std::function<double(const dvec &)> eval_param_loss = [create_params, training_fens, training_results, K](const dvec &x) {
        const EvalParameters params = create_params(x);

        dvec predictions = eval_positions(params, training_fens, K);

        return loss(training_results, predictions);
    };

    // Measure initial verification loss
    dvec verification_predictions_before = eval_positions(p0, verification_fens, K);

    double validation_loss_before = loss(validation_results, verification_predictions_before);

    std::cout << "\n\nNow minimizing parameters" << std::endl;
    std::cout << "Initial parameters:\n" << p0.to_string() << std::endl;
    std::cout << "Validation loss pre fit: " << validation_loss_before << std::endl;

    MixedGradient param_eval(is_float, eval_param_loss);

    // Minimize using training data only
    MinimizeResult param_res = minimize(x0, param_eval);

    EvalParameters optimized_params = create_params(param_res.x);

    // Measure final verification loss
    dvec verification_predictions_after = eval_positions(optimized_params, verification_fens, K);

    double verification_loss_after = loss(validation_results, verification_predictions_after);

    // Results
    std::cout << "\n\n========== RESULTS ==========\n";

    std::cout << "\nOptimized parameters:\n" << optimized_params.to_string() << std::endl;

    std::cout << "\nVerification loss:\n";
    std::cout << "  Before: " << validation_loss_before << std::endl;
    std::cout << "  After:  " << verification_loss_after << std::endl;

    std::cout << "  Change: " << verification_loss_after - validation_loss_before << std::endl;

    if (validation_loss_before != 0.0) {
        double improvement = 100.0 * (validation_loss_before - verification_loss_after) / validation_loss_before;

        std::cout << "  Improvement: " << improvement << "%" << std::endl;
    }

    std::cout << "\n=============================\n";

    return 0;
}