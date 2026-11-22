#include "core/board.h"
#include "eval/hce/parameter_config.h"
#include "parameter_creator.h"
#include "texel/core.h"
#include "texel/eigen_defs.h"
#include "texel/eval_pos.h"
#include "texel/extract_features.h"
#include "texel/input_reader.h"
#include "texel/texel_problem.h"
#include "tuning_config.h"
#include "utils/config.h"
#include <argparse/argparse.hpp>
#include <iomanip>
#include <iostream>
#include <optional>
#include <pagmo/algorithm.hpp>
#include <pagmo/algorithms/de.hpp>
#include <pagmo/algorithms/nlopt.hpp>
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
    parser.add_argument("--ntrain").help("Number of positions to train on..").scan<'i', int>();
    parser.add_argument("--nvalidation").help("Number of positions to validate on.").scan<'i', int>();
    parser.add_argument("--method").help("Method to compute evaluation. [direct, quiesence]. Direct is faster but less accurate.").default_value("direct");
    parser.add_argument("--prior_offset").help("Offset in prior stdev sigma = offset + r * y").default_value(10);
    parser.add_argument("--prior_relunc").help("Relative uncertainty in prior stdev. r in sigma = offset + r * y").default_value(0.3);
    parser.add_argument("--batch_gens").help("Generations per batch before checking validation.").default_value(50).scan<'i', int>();
    parser.add_argument("--patience").help("Stop after N batches without validation improvement.").default_value(2).scan<'i', int>();
    parser.add_argument("--max_gens").help("Maximum total generations.").default_value(10000).scan<'i', int>();
    parser.add_argument("--param_file").help("Default parameters to adjust from.");
    parser.add_argument("--param_config_file").help("Contains settings per parameter");
    parser.add_argument("--save").help("File to save into").default_value("results/tuning/best.json");
    parser.add_argument("--pop_size").help("Population size in minimizer").default_value(500).scan<'i', int>();
    try {
        parser.parse_args(argc, argv);
    } catch (const std::exception &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        std::exit(1);
    }

    std::string path = parser.get<std::string>("file");
    int ntrain = parser.get<int>("--ntrain");
    int nvalidation = parser.get<int>("--nvalidation");
    std::string objective = parser.get<std::string>("--objective");
    std::string method = parser.get<std::string>("--method");
    int prior_offset = parser.get<int>("--prior_offset");
    double prior_relunc = parser.get<double>("--prior_relunc");
    int batch_gens = parser.get<int>("--batch_gens");
    int patience = parser.get<int>("--patience");
    int max_gens = parser.get<int>("--max_gens");
    size_t pop_size = static_cast<size_t>(parser.get<int>("--pop_size"));

    auto cpu_start = std::clock();
    std::string param_file = parser.get<std::string>("--param_file");
    std::string param_config_file = parser.get<std::string>("--param_config_file");
    std::string outfile = parser.get<std::string>("--save");
    // Load dataset

#pragma omp parallel
    {
#pragma omp single
        std::cout << "Active threads: " << omp_get_num_threads() << std::endl;
    }
    std::vector<Position> valid_positions;
    std::vector<std::string> fens;
    int to_load = ntrain + nvalidation;
    valid_positions.reserve(to_load);
    fens.reserve(to_load);
    {

        std::vector<Position> positions = read_dataset(path, {});

        std::cout << "Loaded " << positions.size() << " positions" << std::endl;
        int loaded = 0;

        for (const auto &position : positions) {
            if (loaded >= to_load)
                break;
            Board test;
            if (test.read_fen(position.fen)) {
                valid_positions.push_back(position);
                fens.push_back(position.fen);
                loaded++;
            }
        }
        if (loaded >= to_load) {
            std::cout << "Successfully processed " << fens.size() << " positions" << std::endl;
        } else {
            throw std::runtime_error("Could not load enough positions. Loaded: " + std::to_string(loaded) + " but " + std::to_string(to_load) + " requested.");
        }
    }

    std::vector<Position> training_res(valid_positions.begin(), valid_positions.begin() + ntrain);
    std::vector<Position> validation_res(valid_positions.begin() + ntrain, valid_positions.end());

    std::vector<std::string> training_fens(fens.begin(), fens.begin() + ntrain);
    std::vector<std::string> validation_fens(fens.begin() + ntrain, fens.end());

    std::shared_ptr<SparseDataSet> training_data(std::make_shared<SparseDataSet>(build_dataset(training_fens)));
    std::shared_ptr<SparseDataSet> validation_data(std::make_shared<SparseDataSet>(build_dataset(validation_fens)));
    evaluate_dataset(*training_data);
    std::cout << "\nDataset split:" << "\n  Training:    " << ntrain << "\n  Verification: " << nvalidation << std::endl;
    // Initial parameters
    Eval::HCE::ParameterConfig config(param_file);
    Eval::HCE::EvalParameters p0 = config.params; // initial parameters
    Texel::TuningConfig tuning_config(param_config_file);
    ParameterCreator param_creator(p0, tuning_config);

    double K = 1.2;
    // Evaluation constant
    auto to_res = [objective](std::vector<Position> &res, double K) {
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
    pagmo::vector_double x0(param_creator.get_nlocal(), 0.0);
    int num_params = x0.size();
    dvec training_results = to_res(training_res, K);

    if (objective == "result") { // for objective=eval, K defines the target, so don't fit it
        auto loss_at = [&](double k) {
            TexelProblem w(param_creator, training_data, training_results, k);
            return w.fitness(x0)[0];
        };

        // Golden-section search; the loss is unimodal in K.
        const double phi = (std::sqrt(5.0) - 1.0) / 2.0;
        double a = 0.05, b = 3.0; // adjust to your sigmoid's scaling
        double c = b - phi * (b - a), d = a + phi * (b - a);
        double fc = loss_at(c), fd = loss_at(d);
        while (b - a > 1e-3) {
            if (fc < fd) {
                b = d;
                d = c;
                fd = fc;
                c = b - phi * (b - a);
                fc = loss_at(c);
            } else {
                a = c;
                c = d;
                fc = fd;
                d = a + phi * (b - a);
                fd = loss_at(d);
            }
        }
        K = 0.5 * (a + b);
        std::cout << "Fitted K = " << K << std::endl;
    }
    // redo training results!
    training_results = to_res(training_res, K);
    dvec validation_results = to_res(validation_res, K);

    TexelProblem training(param_creator, training_data, training_results, K);
    TexelProblem validation(param_creator, validation_data, validation_results, K);

    double training_loss_before = training.fitness(x0)[0];
    double validation_loss_before = validation.fitness(x0)[0];

    std::cout << "\n\nNow minimizing " << num_params << " parameters\n" << std::endl;
    std::cout << "Validation loss pre fit: " << std::setprecision(5) << validation_loss_before << std::endl;
    pagmo::problem prob{training};

    pagmo::nlopt lbfgs{"lbfgs"};
    lbfgs.set_maxeval(5000);
    lbfgs.set_ftol_rel(1e-9);
    lbfgs.set_xtol_rel(1e-7);

    pagmo::algorithm algo{lbfgs};

    const int seed = 42;
    pagmo::population pop{prob, 1, seed};
    pop.set_x(0, x0);
    pop = algo.evolve(pop);

    pagmo::vector_double best_x = pop.champion_x();

    double train_loss = training.fitness(best_x)[0]; 
    double val_loss = validation.fitness(best_x)[0];

    std::cout << "Initial train loss: " << training.fitness(x0)[0] << '\n';

    std::cout << "Final train loss:   " << train_loss << '\n';

    std::cout << "Initial val loss:   " << validation_loss_before << '\n';

    std::cout << "Final val loss:     " << val_loss << '\n';

    // ============================================================
    // 8. Convert local parameters into global evaluation parameters
    // ============================================================

    Eigen::Map<const Eigen::VectorXd> local_best(best_x.data(), best_x.size());

    ParameterVector global_best = param_creator.create_global_params(local_best);

    Eval::HCE::EvalParameters best(global_best);
    std::cout <<best.to_string() <<std::endl;

    // Current date
    std::time_t now = std::time(nullptr);
    std::tm *local_time = std::localtime(&now);

    std::ostringstream date_stream;
    date_stream << std::put_time(local_time, "%Y-%m-%d");

    // CPU time (not wall time)
    double cpu_time = static_cast<double>(std::clock() - cpu_start) / CLOCKS_PER_SEC;

    // Optimization metadata
    nlohmann::json stats = {{"date", date_stream.str()},

                            {"training_loss", {{"before", training_loss_before}, {"after", train_loss}}},

                            {"validation_loss", {{"before", validation_loss_before}, {"after", val_loss}}},

                            {"cpu_time_seconds", cpu_time},

                            {"dataset", path},

                            {"training_points", ntrain},
                            {"validation_points", validation_fens.size()},

                            // Input settings
                            {"settings",
                             {{"objective", objective},
                              {"method", method},

                              {"nload_requested", to_load},
                              {"nvalidation_requested", nvalidation},

                              {"prior_offset", prior_offset},
                              {"prior_relunc", prior_relunc},

                              {"param_file", param_file},
                              {"param_config_file", param_config_file},

                              {"population_size", pop_size},
                              {"seed", seed}}}};

    std::cout << "\nOptimization complete!" << std::endl;
    std::cout << "Best training loss:      " << train_loss << std::endl;
    std::cout << "Validation loss before:  " << validation_loss_before << std::endl;
    std::cout << "Best validation loss:    " << val_loss << std::endl;
    std::cout << "CPU time:                " << cpu_time << " seconds" << std::endl;

    std::cout << "\n=============================\n";

    config.params = best;
    config.save(outfile, stats);

    std::cout << "Saved parameters to " << outfile << std::endl;

    return 0;
}