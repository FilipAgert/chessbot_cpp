#include "texel/core.h"
#include "texel/eigen_defs.h"
#include "texel/eval_pos.h"
#include "texel/input_reader.h"
#include "texel/pagmo_fitness.h"
#include "utils/config.h"
#include <argparse/argparse.hpp>
#include <iostream>
#include <optional>
#include <pagmo/algorithm.hpp>
#include <pagmo/algorithms/sga.hpp>
#include <pagmo/population.hpp>
#include <pagmo/problem.hpp>
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
    for (int i = 0; i < argc; ++i) {
        std::cout << "argv[" << i << "]: " << argv[i] << '\n';
    }
    std::cout.flush();
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
    parser.add_argument("--prior_offset").help("Offset in prior stdev sigma = offset + r * y").default_value(10);
    parser.add_argument("--prior_relunc").help("Relative uncertainty in prior stdev. r in sigma = offset + r * y").default_value(0.3);
    parser.add_argument("--batch_gens").help("Generations per batch before checking validation.").default_value(50).scan<'i', int>();
    parser.add_argument("--patience").help("Stop after N batches without validation improvement.").default_value(2).scan<'i', int>();
    parser.add_argument("--max_gens").help("Maximum total generations.").default_value(10000).scan<'i', int>();

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
    int prior_offset = parser.get<int>("--prior_offset");
    double prior_relunc = parser.get<double>("--prior_relunc");
    int batch_gens = parser.get<int>("--batch_gens");
    int patience = parser.get<int>("--patience");
    int max_gens = parser.get<int>("--max_gens");

    // Load dataset
    std::vector<Position> positions = read_dataset(path, {nload});

    std::cout << "Loaded " << positions.size() << " positions" << std::endl;

#pragma omp parallel
    {
#pragma omp single
        std::cout << "Active threads: " << omp_get_num_threads() << std::endl;
    }

    std::vector<Position> valid_positions;
    std::vector<std::string> fens;
    valid_positions.reserve(positions.size());
    fens.reserve(positions.size());

    for (const auto &position : positions) {
        Board test;
        if (test.read_fen(position.fen)) {
            valid_positions.push_back(position);
            fens.push_back(position.fen);
        }
    }

    std::cout << "Successfully processed " << fens.size() << " positions" << std::endl;

    // Split training and verification data
    if (nvalidation >= static_cast<int>(fens.size())) {
        std::cerr << "Error: Verification set must be smaller than the dataset.\n";
        return 1;
    }

    const size_t training_count = fens.size() - nvalidation;
    std::vector<Position> training_res(valid_positions.begin(), valid_positions.begin() + training_count);
    std::vector<Position> validation_res(valid_positions.begin() + training_count, valid_positions.end());

    std::vector<std::string> training_fens(fens.begin(), fens.begin() + training_count);
    std::vector<std::string> validation_fens(fens.begin() + training_count, fens.end());

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
    Eval::HCE::EvalParameters p0;

    int num_params = p0.num_params();
    std::vector<bool> is_float(num_params, false);

    auto create_params = [is_float, p0](const pagmo::vector_double &x) {
        Eval::HCE::EvalParameters params;
        std::vector<int> p(x.size());

        for (size_t i = 0; i < x.size(); i++) {
            p.at(i) = round_if_int(x.at(i), is_float[i]);
        }
        params.from_vector(p);
        return params;
    };

    pagmo::vector_double x0(num_params);
    std::vector<int> params = p0.to_vector();
    for (size_t i = 0; i < x0.size(); i++) {
        x0[i] = params[i];
    }

    PagmoLossWrapper training(training_fens, K, training_results, p0);
    PagmoLossWrapper validation(validation_fens, K, validation_results, p0);

    double validation_loss_before = validation.fitness(x0)[0];

    std::cout << "\n\nNow minimizing parameters" << std::endl;
    std::cout << "Initial parameters:\n" << p0.to_string() << std::endl;
    std::cout << "Validation loss pre fit: " << validation_loss_before << std::endl;
    pagmo::problem prob{training};
    pagmo::population pop{prob, 100, 42};

    // Seed the population with your initial parameters x0
    pop.set_x(0, x0);

    pagmo::algorithm algo{pagmo::sga(batch_gens,   // generations per batch
                                     0.9,          // crossover probability
                                     2.0,          // crossover distribution index
                                     0.01,         // mutation probability
                                     10.0,         // mutation distribution index
                                     2,            // tournament size
                                     "sbx",        // crossover
                                     "polynomial", // mutation
                                     "tournament", // selection
                                     42            // seed
                                     )};
    algo.set_verbosity(0); // Batch summary logs are cleaner than per-generation logs

    pagmo::vector_double best_x = x0;
    double best_val_loss = validation_loss_before;
    double best_train_loss = training.fitness(x0)[0];
    int no_improve_batches = 0;
    int total_gens = 0;

    std::cout << "Starting batched evolution (batch_gens=" << batch_gens << ", patience=" << patience << ", max_gens=" << max_gens << ")\n";

    while (total_gens < max_gens && no_improve_batches < patience) {
        pop = algo.evolve(pop);
        total_gens += batch_gens;

        pagmo::vector_double curr_x = pop.champion_x();
        double curr_train_loss = pop.champion_f()[0];
        double curr_val_loss = validation.fitness(curr_x)[0];

        if (curr_val_loss < best_val_loss) {
            best_val_loss = curr_val_loss;
            best_train_loss = curr_train_loss;
            best_x = curr_x;
            no_improve_batches = 0;
            std::cout << "[Gen " << total_gens << "] NEW BEST | Train: " << curr_train_loss << " | Val: " << curr_val_loss << '\n';
        } else {
            no_improve_batches++;
            std::cout << "[Gen " << total_gens << "] No improvement (" << no_improve_batches << "/" << patience << ") | Train: " << curr_train_loss
                      << " | Val: " << curr_val_loss << " (Best Val: " << best_val_loss << ")\n";
        }
    }

    if (no_improve_batches >= patience) {
        std::cout << "\nEarly stopping triggered after " << total_gens << " generations.\n";
    } else {
        std::cout << "\nReached max generations (" << max_gens << ").\n";
    }

    // Convert best_x back to your parameter structure
    Eval::HCE::EvalParameters optimized_params = create_params(best_x);

    std::cout << "\nOptimization complete!" << std::endl;
    std::cout << "Best training loss:      " << best_train_loss << std::endl;
    std::cout << "Validation loss before:  " << validation_loss_before << std::endl;
    std::cout << "Best validation loss:    " << best_val_loss << std::endl;

    std::cout << "Optimized parameters:\n" << optimized_params.to_string() << std::endl;
    std::cout << "\n=============================\n";

    return 0;
}