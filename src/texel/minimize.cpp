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

MinimizeResult conjugate_gradient(const Gradient &calc, dvec x0, int max_steps, double tol, std::optional<MinimizeResult> failed = std::nullopt) {
    if (failed && failed->flag == MinimizeResult::success)
        return failed.value();

    dvec x = x0;
    auto [f, grad, H] = calc.eval_grad_hessian(x);

    if (grad.norm() < tol)
        return {x, f, grad, H, MinimizeResult::success};

    const int n = static_cast<int>(x.size());

    // Initial direction: Newton if descent, otherwise steepest descent.
    dvec p = -grad;

    Eigen::FullPivLU<dmat> lu(H);
    if (lu.isInvertible()) {
        dvec pn = lu.solve(-grad);
        if (grad.dot(pn) < 0)
            p = pn;
    }

    // Unit-length direction.
    p.normalize();

    for (int step = 0; step < max_steps; ++step) {

        // Exactly one unit step.
        dvec x_next = x + p;

        double f_next = calc.evaluate(x_next);
        dvec grad_next = calc.evaluate_grad(x_next);

        // If the step increases the objective, try steepest descent.
        if (f_next >= f) {
            p = -grad;
            p.normalize();

            x_next = x + p;
            f_next = calc.evaluate(x_next);
            grad_next = calc.evaluate_grad(x_next);

            // No improvement: terminate.
            if (f_next >= f)
                return {x, f, grad, H, MinimizeResult::max_steps};
        }

        // Polak-Ribiere+
        double beta = grad_next.dot(grad_next - grad) / grad.squaredNorm();

        beta = std::max(0.0, beta);

        bool restart = ((step + 1) % n == 0);

        dvec p_next = -grad_next + (restart ? 0.0 : beta) * p;

        if (p_next.norm() < 1e-14)
            return {x_next, f_next, grad_next, H, MinimizeResult::success};

        p = p_next.normalized();

        x = x_next;
        f = f_next;
        grad = grad_next;

        std::cout << "CG step " << step << " | f: " << f << " | ||grad||: " << grad.norm() << std::endl;

        if (grad.norm() < tol)
            return {x, f, grad, H, MinimizeResult::success};
    }

    return {x, f, grad, H, MinimizeResult::max_steps};
}

MinimizeResult minimize(const dvec &x0, const Gradient &calc, std::function<std::string(dvec)> print_fnc) {
    const int NR_freeze = 10;
    const int max_outer = 1000;
    const int max_stalls = 3;
    double tol = 1e-6;
    std::string tab = "    ";

    double f0 = calc.evaluate(x0);
    auto [f, grad, H] = calc.eval_grad_hessian(x0);
    MinimizeResult best{x0, f, grad, H, MinimizeResult::max_steps};
    if (grad.norm() < tol)
        best.flag = MinimizeResult::success;

    std::cout << "Starting minimizer with " << x0.size() << " params\n" << std::setprecision(6) << print_fnc(x0) << std::endl;

    int no_nr_cnt = 0, stalls = 0, i = 0;
    while (best.flag != MinimizeResult::success && i < max_outer && stalls < max_stalls) {
        bool use_nr = (no_nr_cnt >= NR_freeze);
        std::cout << i << ": " << (use_nr ? "newton-raphson" : "conjugate gradient") << " from f=" << best.f << ", |df|^2=" << best.grad.squaredNorm()
                  << std::endl;

        MinimizeResult res = use_nr ? newton_raphson(best.x, calc, 5, tol) : conjugate_gradient(calc, best.x, 100, tol);
        std::cout << tab << "-> f=" << res.f << ", |df|^2=" << res.grad.squaredNorm() << std::endl;

        // Accept only genuine improvements (or success)
        bool improved = res.f < best.f - 1e-14 * std::max(1.0, std::abs(best.f));
        if (improved || res.flag == MinimizeResult::success) {
            best = res;
            stalls = 0;
        } else {
            stalls++;
            std::cout << tab << "No improvement (" << stalls << "/" << max_stalls << ")\n";
        }
        no_nr_cnt = use_nr ? 0 : no_nr_cnt + 1;
        i++;
    }

    if (best.flag != MinimizeResult::success)
        std::cout << "Warning: stopped without converging (stalled or max outer iterations)\n";

    std::cout << "Minimization complete. f=" << best.f << ", an improvement of " << std::abs(best.f - f0) << "\n";
    std::cout << "Iterations: " << i << "\nEvaluation function calls: " << calc.num_evals << "\nGradient calls: " << calc.num_grads
              << "\nHessian function calls: " << calc.num_h << "\n";
    return best;
}
}  // namespace Texel