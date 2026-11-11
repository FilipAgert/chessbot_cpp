#include "texel/minimize.h"
#include "texel/gradient.h"
#include <Eigen/LU>
#include <iomanip>
#include <ios>
#include <iostream>
#include <optional>
#include <stdexcept>
namespace Texel {
MinimizeResult newton_raphson(const dvec &x0, const Gradient &calc, int max_steps, double tol) {
    dvec x = x0;
    dvec grad;
    dmat H;
    double f = 0.0;
    for (int step = 0; step < max_steps; ++step) {
        auto res = calc.eval_grad_hessian(x);
        f = std::get<0>(res);
        grad = std::get<1>(res);
        H = std::get<2>(res);

        if (grad.norm() < tol) {
            return {x, f, grad, H, MinimizeResult::success};
        }

        Eigen::FullPivLU<dmat> lu(H);
        if (!lu.isInvertible()) {
            return {x, f, grad, H, MinimizeResult::hessian_non_invertible};
        }

        dvec delta = lu.solve(-grad);

        double alpha = 1.0;  // Try a full Newton step first
        double c = 1e-4;     // Armijo condition parameter
        double f_new = f;
        dvec x_new = x;

        int line_search_steps = 0;
        while (line_search_steps < 10) {
            x_new = x + alpha * delta;
            f_new = std::get<0>(calc.eval_grad_hessian(x_new));  // Evaluate loss at new point

            // Check if loss improved sufficiently (Armijo condition)
            if (f_new <= f + c * alpha * grad.dot(delta)) {
                break;  // Valid step found!
            }
            alpha *= 0.5;  // Shrink step size
            line_search_steps++;
        }

        // Update x with the safe, scaled step
        x = x_new;
        std::cout << "Step " << step << " | f: " << f << " | alpha: " << alpha << " | ||delta||: " << delta.norm() << std::endl;
    }

    return {x, f, grad, H, MinimizeResult::max_steps};
}
double backtracking_line_search(const dvec &x, double f_current, const dvec &grad, const dvec &p, const Gradient &calc, double initial_alpha = 1.0,
                                double c1 = 1e-4,  // Armijo condition parameter (usually 1e-4)
                                double rho = 0.5   // Shrink factor (cut step in half each time)
) {
    double alpha = initial_alpha;
    double directional_deriv = grad.dot(p);

    // Safety check: CG directions should always be descent directions (grad^T * p < 0).
    // If it's positive, p is pointing uphill. Force a reset to the negative gradient.
    if (directional_deriv >= 0) {
        return 1e-5;  // Fallback to a tiny step or trigger a CG restart
    }

    int max_iters = 20;
    for (int i = 0; i < max_iters; ++i) {
        dvec x_new = x + alpha * p;

        // Evaluate loss at the candidate point
        auto res = calc.eval_grad_hessian(x_new);  // (f, grad, H)
        double f_new = std::get<0>(res);

        // Armijo Condition: Check if f decreased *sufficiently*
        if (f_new <= f_current + c1 * alpha * directional_deriv) {
            return alpha;  // Valid step size found!
        }

        alpha *= rho;  // Step was too big; shrink it
    }

    // If line search exhausts iterations, return the smallest shrunk alpha
    return alpha;
}

MinimizeResult conjugate_gradient(const Gradient &calc, dvec x0, int max_steps, double tol, std::optional<MinimizeResult> failed = std::nullopt) {
    dvec x = x0;
    dvec grad;
    double f = calc.evaluate(x);

    // Use provided gradient or evaluate it
    dmat H;
    if (failed) {
        if (failed->flag == MinimizeResult::success) {
            return failed.value();
        }
        grad = failed->grad;
        H = failed->H;
    } else {
        auto res = calc.eval_grad_hessian(x);
        f = std::get<0>(res);
        grad = std::get<1>(res);
        H = std::get<2>(res);
    }

    // Keep track of the Hessian (or fallback identity if none provided)

    // Initialize search direction using initial Hessian preconditioner if available
    dvec p;
    Eigen::FullPivLU<dmat> lu(H);
    if (lu.isInvertible()) {
        p = lu.solve(-grad);
    } else {
        p = -grad;  // Fallback if singular
    }

    double r_old_norm_sq = grad.squaredNorm();
    if (std::sqrt(r_old_norm_sq) < tol) {
        return {x, f, grad, H, MinimizeResult::success};
    }

    for (int step = 0; step < max_steps; ++step) {
        // Step size / line search placeholder (adjust alpha or implement backtracking)
        double alpha = 300;  // backtracking_line_search(x0, f, grad, p, calc, 1);

        dvec x_next = x + alpha * p;
        dvec grad_next;
        grad_next = calc.evaluate_grad(x_next);

        if (grad_next.norm() < tol) {
            f = calc.evaluate(x_next);
            return {x_next, f, grad_next, H, MinimizeResult::success};
        }

        // Polak-Ribière / Fletcher-Reeves beta calculation
        double r_new_norm_sq = grad_next.squaredNorm();
        double beta = r_new_norm_sq / r_old_norm_sq;

        p = -grad_next + beta * p;

        x = x_next;
        f = calc.evaluate(x);
        grad = grad_next;
        r_old_norm_sq = r_new_norm_sq;
        std::cout << "Step " << step << " | alpha " << alpha << " | f: " << f << " | ||grad||: " << grad.norm() << " | ||delta||: " << (alpha * p).norm()
                  << std::endl;
    }
    f = calc.evaluate(x);
    return {x, f, grad, H, MinimizeResult::max_steps};
}

MinimizeResult minimize(const dvec &x0, const Gradient &calc, std::function<std::string(dvec)> print_fnc) {
    dvec x = x0;
    const int NR_freeze = 10;
    int no_nr_cnt = 0;
    double f0 = calc.evaluate(x0);
    double f = f0;
    dvec grad = calc.evaluate_grad(x0);
    double tol = 1e-6;
    std::cout << "Starting minimizer with " << static_cast<int>(x0.size()) << " params\n" << std::setprecision(6) << print_fnc(x0) << std::endl;

    std::optional<MinimizeResult> res_ = std::nullopt;
    int i = 0;
    std::string tab = "    ";
    while ((!res_.has_value()) || ((res_.has_value()) && res_.value().flag != MinimizeResult::success)) {

        if (no_nr_cnt >= NR_freeze) {
            // newton-raphson test
            std::cout << i << ": performing newton-raphson from f(x_0)=" << f << ", |df|^2=" << grad.squaredNorm() << std::endl;
            MinimizeResult res = newton_raphson(x, calc, 5, tol);
            std::cout << tab << calc.num_evals << " evals perfomed. f(x_1)=" << res.f << ", |df|^2=" << res.grad.squaredNorm() << std::endl;
            if (res.f < f) {
                f = res.f;
                grad = res.grad;
                x = res.x;
                res_ = std::make_optional(res);
            } else {
                std::cout << tab << "Switching to conjugate gradient due to non-improvement. " << std::endl;
            }
            if (res.flag == MinimizeResult::hessian_non_invertible) {
                std::cout << tab << "Switching to conjugate gradient due to non-invertible hessian. " << std::endl;
            }
            no_nr_cnt = 0;
        } else {
            std::cout << i << ": performing conjugate gradient from f(x_0)=" << f << ", |df|^2=" << grad.squaredNorm() << std::endl;
            MinimizeResult res = conjugate_gradient(calc, x, 100, tol, res_);
            std::cout << tab << "Ran conjugate gradient to f(x_1)=" << res.f << ", |df|^2=" << res.grad.squaredNorm() << "\n" << print_fnc(res.x) << std::endl;
            if (res.flag != MinimizeResult::success) {
                if (res.f < f) {
                    f = res.f;
                    grad = res.grad;
                    x = res.x;
                    auto c = calc.eval_grad_hessian(x);
                    res.H = std::get<2>(c);
                    res_ = {res};
                } else {
                    res_ = std::nullopt;
                }
            }
            no_nr_cnt++;
        }
        i++;
    }
    if (!res_) {
        throw std::runtime_error("No result");
    }
    MinimizeResult res = res_.value();

    std::cout << "Minimization complete. f=" << res.f << ", an improvement of " << std::abs(res.f - f0) << "\n";
    std::cout << "Final parameters:\n" << print_fnc(res.x) << "\n";
    std::cout << "Iterations: " << i << "\n"
              << "Evaluation function calls: " << calc.num_evals << "\n"
              << "Gradient calls: " << calc.num_grads << "\n"
              << "Hessian function calls: " << calc.num_h << "\n";
    return res;
}
}  // namespace Texel