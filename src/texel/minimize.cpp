#include "texel/minimize.h"
#include "texel/gradient.h"
#include <Eigen/LU>
#include <iomanip>
#include <ios>
#include <iostream>
#include <optional>
#include <stdexcept>
namespace Texel {
// Armijo backtracking. Returns 0 if no acceptable step was found.
double backtracking_line_search(const dvec &x, double f_current, const dvec &grad, const dvec &p, const Gradient &calc, double initial_alpha = 1.0,
                                double c1 = 1e-4, double rho = 0.5) {
    double dd = grad.dot(p);
    if (dd >= 0)
        return 0.0;  // not a descent direction

    double alpha = initial_alpha;
    for (int i = 0; i < 40; ++i) {
        double f_new = calc.evaluate(x + alpha * p);  // cheaper than eval_grad_hessian
        if (std::isfinite(f_new) && f_new <= f_current + c1 * alpha * dd)
            return alpha;
        alpha *= rho;
    }
    return 0.0;  // failed: caller must handle
}

MinimizeResult newton_raphson(const dvec &x0, const Gradient &calc, int max_steps, double tol) {
    dvec x = x0;
    auto [f, grad, H] = calc.eval_grad_hessian(x);

    for (int step = 0; step < max_steps; ++step) {
        if (grad.norm() < tol)
            return {x, f, grad, H, MinimizeResult::success};

        Eigen::FullPivLU<dmat> lu(H);
        if (!lu.isInvertible())
            return {x, f, grad, H, MinimizeResult::hessian_non_invertible};

        dvec delta = lu.solve(-grad);
        if (grad.dot(delta) >= 0)
            delta = -grad;  // indefinite Hessian -> fall back to steepest descent

        double alpha = backtracking_line_search(x, f, grad, delta, calc, 1.0);
        if (alpha == 0.0)
            break;  // no improvement possible; keep current (consistent) x, f, grad, H

        x += alpha * delta;
        std::tie(f, grad, H) = calc.eval_grad_hessian(x);  // keep f/grad/H consistent with x
        std::cout << "NR step " << step << " | f: " << f << " | alpha: " << alpha << " | ||delta||: " << delta.norm() << std::endl;
    }
    if (grad.norm() < tol)
        return {x, f, grad, H, MinimizeResult::success};
    return {x, f, grad, H, MinimizeResult::max_steps};
}

MinimizeResult conjugate_gradient(const Gradient &calc, const Gradient &validation, dvec x0, int max_steps, double tol,
                                  std::optional<MinimizeResult> failed = std::nullopt) {

    if (failed && failed->flag == MinimizeResult::success)
        return failed.value();

    dvec x = x0;
    double f = calc.evaluate(x);
    dvec grad = calc.evaluate_grad(x);
    dmat H = dmat::Zero(x.size(), x.size());

    double best_val_f = validation.evaluate(x);
    MinimizeResult best_val{x, f, grad, H, MinimizeResult::max_steps};

    if (grad.norm() < tol) {
        best_val.flag = MinimizeResult::success;
        return best_val;
    }

    const int n = static_cast<int>(x.size());
    dvec p = -grad.normalized();

    constexpr double min_step = 0.1;
    constexpr double max_step = 50.0;
    constexpr int val_check_interval = 1;  // Check validation every 5 CG steps
    constexpr int val_patience = 20;       // Stop if no val improvement for 20 steps
    double step_size = 1.0;
    int steps_since_val_best = 0;

    for (int step = 0; step < max_steps; ++step) {
        dvec x_next;
        double f_next = f;
        bool accepted = false;

        // 1. Backtrack step_size if training objective increases
        while (true) {
            x_next = x + step_size * p;
            f_next = calc.evaluate(x_next);

            if (f_next < f) {
                accepted = true;
                break;
            }
            if (step_size <= min_step)
                break;

            step_size = std::max(min_step, step_size * 0.5);
        }

        // 2. Fallback to steepest descent if conjugate direction stalls
        if (!accepted) {
            p = -grad.normalized();
            step_size = 1.0;

            while (true) {
                x_next = x + step_size * p;
                f_next = calc.evaluate(x_next);

                if (f_next < f) {
                    accepted = true;
                    break;
                }
                if (step_size <= min_step)
                    break;

                step_size = std::max(min_step, step_size * 0.5);
            }

            if (!accepted)
                return best_val;
        }

        dvec grad_next = calc.evaluate_grad(x_next);

        // Polak-Ribiere+
        double beta = grad_next.dot(grad_next - grad) / grad.squaredNorm();
        beta = std::max(0.0, beta);

        bool restart = ((step + 1) % n == 0);
        dvec p_next = -grad_next + (restart ? 0.0 : beta) * p;

        if (p_next.norm() < 1e-14 || grad_next.dot(p_next) >= 0.0) {
            p = -grad_next.normalized();
        } else {
            p = p_next.normalized();
        }

        x = x_next;
        f = f_next;
        grad = grad_next;
        step_size = std::clamp(step_size * 1.25, min_step, max_step);

        // Periodic validation check
        if ((step + 1) % val_check_interval == 0 || grad.norm() < tol) {
            double val_f = validation.evaluate(x);

            if (val_f < best_val_f) {
                best_val_f = val_f;
                best_val = {x, f, grad, H, MinimizeResult::max_steps};
                steps_since_val_best = 0;
            } else {
                steps_since_val_best += val_check_interval;
            }

            std::cout << "CG step " << step << " | train: " << f << " | val: " << val_f << " (best: " << best_val_f << ")" << " | ||grad||: " << grad.norm()
                      << " | step: " << step_size << std::endl;

            if (steps_since_val_best >= val_patience) {
                std::cout << "CG early stop: validation loss stopped improving.\n";
                return best_val;
            }
        }

        if (grad.norm() < tol) {
            best_val.flag = MinimizeResult::success;
            return best_val;
        }
    }

    return best_val;
}
MinimizeResult minimize(const dvec &x0, const Gradient &calc, const Gradient &validation) {
    const int NR_freeze = 10;
    const int max_outer = 1000;
    const int max_stalls = 3;
    const int max_val_stalls = 3;  // Stop outer loop if validation degrades across 3 outer cycles
    double tol = 1e-6;
    std::string tab = "    ";

    double f0 = calc.evaluate(x0);
    double f0_validation = validation.evaluate(x0);
    dvec grad = calc.evaluate_grad(x0);
    dmat H;

    MinimizeResult best{x0, f0, grad, H, MinimizeResult::max_steps};
    MinimizeResult best_val_res = best;
    double best_val_f = f0_validation;

    if (grad.norm() < tol) {
        best.flag = MinimizeResult::success;
        return best;
    }

    std::cout << "Starting minimizer with " << x0.size() << " params" << " | initial train f=" << f0 << " | initial val f=" << f0_validation << std::endl;

    int stalls = 0, val_stalls = 0, i = 0;
    while (best.flag != MinimizeResult::success && i < max_outer && stalls < max_stalls && val_stalls < max_val_stalls) {
        std::cout << i << ": " << "conjugate gradient" << " from f=" << best.f << ", |df|^2=" << best.grad.squaredNorm() << std::endl;

        MinimizeResult res = conjugate_gradient(calc, validation, best.x, 100, tol);

        double val_f = validation.evaluate(res.x);
        std::cout << tab << "-> train f=" << res.f << ", val f=" << val_f << ", |df|^2=" << res.grad.squaredNorm() << std::endl;

        // Track training progress to keep optimizer moving
        bool improved = res.f < best.f - 1e-14 * std::max(1.0, std::abs(best.f));
        if (improved || res.flag == MinimizeResult::success) {
            best = res;
            stalls = 0;
        } else {
            stalls++;
            std::cout << tab << "No train improvement (" << stalls << "/" << max_stalls << ")\n";
        }

        // Track best generalizable parameters on validation set
        if (val_f < best_val_f - 1e-14 * std::max(1.0, std::abs(best_val_f))) {
            best_val_f = val_f;
            best_val_res = res;
            val_stalls = 0;
            std::cout << tab << "New best validation f=" << best_val_f << "\n";
        } else {
            val_stalls++;
            std::cout << tab << "No validation improvement (" << val_stalls << "/" << max_val_stalls << ")\n";
        }

        i++;
    }

    if (val_stalls >= max_val_stalls)
        std::cout << "Stopped early due to validation plateau/overtraining.\n";
    else if (best.flag != MinimizeResult::success)
        std::cout << "Warning: stopped without converging (stalled or max outer iterations)\n";

    std::cout << "Minimization complete.\n"
              << "  Best train f=" << best.f << " (delta: " << std::abs(best.f - f0) << ")\n"
              << "  Best val   f=" << best_val_f << " (delta: " << std::abs(best_val_f - f0_validation) << ")\n"
              << "  Outer iterations: " << i << "\n"
              << "  Train evals: " << calc.num_evals << " | Val evals: " << validation.num_evals << "\n"
              << "  Gradient calls: " << calc.num_grads << " | Hessian calls: " << calc.num_h << "\n";

    return best_val_res;
}
}  // namespace Texel