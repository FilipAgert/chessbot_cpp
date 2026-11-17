#include "eval/hce/parameters.h"
#include <sstream>
namespace Eval::HCE {
std::vector<int> EvalParameters::to_vector() const {
    std::vector<int> v;
#define X(name, def) v.push_back(name);
    EVAL_SCALAR_PARAMS(X)
#undef X
    for (size_t i = 1; i < passed_dist_to_promotion.size(); i++)
        v.push_back(passed_dist_to_promotion[i]);
    for (size_t p = 2; p < num_pieces; ++p)
        v.push_back(piecevals[p]);
    for (size_t p = 1; p < num_pieces; ++p)
        v.push_back(movevals[p]);
    for (size_t p = 1; p < num_pieces; ++p)
        v.insert(v.end(), psts[p].params.begin(), psts[p].params.end());
    for (size_t p = 1; p < num_pieces; ++p) {
        if (psts_endgame[p]) {
            v.insert(v.end(), psts_endgame[p].value().params.begin(), psts_endgame[p].value().params.end());
        }
    }
    return v;
}

void EvalParameters::from_vector(const std::vector<int> &v) {
    if (v.size() != num_params())
        throw std::invalid_argument("EvalParameters::from_vector: wrong size");
    size_t i = 0;
#define X(name, def) name = v[i++];
    EVAL_SCALAR_PARAMS(X)
#undef X
    for (size_t j = 1; j < passed_dist_to_promotion.size(); j++)
        passed_dist_to_promotion[j] = v[i++];
    for (size_t p = 2; p < num_pieces; ++p)
        piecevals[p] = v[i++];
    for (size_t p = 1; p < num_pieces; ++p)
        movevals[p] = v[i++];
    for (size_t p = 1; p < num_pieces; ++p) {
        const size_t n = psts[p].size();
        psts[p].set(psts[p].symmetry(), p, std::vector<int>(v.begin() + i, v.begin() + i + n));
        i += n;
    }
    for (size_t p = 1; p < num_pieces; ++p) {
        if (!psts_endgame[p])
            continue;
        const size_t n = psts_endgame[p].value().size();
        psts_endgame[p].value().set(psts_endgame[p].value().symmetry(), p, std::vector<int>(v.begin() + i, v.begin() + i + n));
        i += n;
    }
}

size_t EvalParameters::num_params() const { return to_vector().size(); }
std::string EvalParameters::to_string() const {
    std::ostringstream os;
#define X(name, def) os << #name << " = " << name << "\n";
    EVAL_SCALAR_PARAMS(X)
#undef X
    auto dump = [&]<size_t N>(const char *label, const std::array<int, N> &a) {
        os << label << " = {";
        for (size_t p = 1; p < N; ++p)
            os << a[p] << (p + 1 < N ? ", " : "");
        os << "}\n";
    };
    dump("dist2promo[1,2,3,4,5,6]", passed_dist_to_promotion);
    dump("piecevals[K,Q,R,N,B,P]", piecevals);
    dump("movevals[K,Q,R,N,B,P]", movevals);
    static const char *names[num_pieces] = {"", "King", "Queen", "Rook", "Knight", "Bishop", "Pawn"};
    for (size_t p = 1; p < num_pieces; ++p) {
        os << "\n" << names[p] << " PST\n";
        os << psts[p].to_string();
        os << "\n";
    }
    for (size_t p = 1; p < num_pieces; ++p) {
        if (!psts_endgame[p])
            continue;
        os << "\n" << names[p] << " PST_ENDGAME\n";
        os << psts_endgame[p].value().to_string();
        os << "\n";
    }
    return os.str();
}
} // namespace Eval::HCE