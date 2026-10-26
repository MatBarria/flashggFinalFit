#include "../interface/RooDYCoreMod.h"
#include "../interface/DYCoreParameters.h"

#include <cmath>


// ============================================================
// Local helper: the fixed RooDYCore shape.
//
// This is a verbatim copy of the fixed shape implemented in
// RooDYCore::evaluate() (Chebyshev Lqq * EW theory sigma summed
// over u, d, s quarks).
//
// It is duplicated here on purpose so that the frozen "core"
// classes stay untouched.  If the fixed core is ever updated,
// remember to update both places.
// ============================================================

namespace {

double dyCoreShape(double mass)
{
    // ===== �������� Chebyshev ϵ�� =====
    static const double cu[7] = {5.65683e+06, -203204, 1628.22, -7.19829, 0.0182027, -2.47312e-05, 1.40276e-08};
    static const double cd[7] = {4.62365e+06, -166744, 1337.52, -5.91684, 0.0149719, -2.03548e-05, 1.15522e-08};
    static const double cs[7] = {2.68735e+06, -100109, 816.659, -3.64815, 0.00928968, -1.26864e-05, 7.22469e-09};

    // --- Chebyshev ---
    auto ChebT = [](int n, double x){
        return std::cos(n * std::acos(x));
    };
    auto Lqq = [&](double m, const double* c){
        double x = (2 * m - (190. + 70.)) / (190. - 70.);
        double s = 0;
        for (int i = 0; i <= 6; i++) s += c[i] * ChebT(i, x);
        return s;
    };

    // --- EW theory ---
    constexpr double alpha = 1. / 137.035999;
    constexpr double GF = 1.1663787e-5;
    constexpr double mZ = 91.1876;
    constexpr double gZ = 2.4952;
    constexpr int Nc = 3;
    constexpr double sw2 = 0.23126;

    auto kappa = [&](){
        return std::sqrt(2.) * GF * mZ * mZ / (4 * M_PI * alpha);
    };

    auto xZ = [&](double s){
        double d = (s - mZ * mZ) * (s - mZ * mZ) + gZ * gZ * mZ * mZ;
        return kappa() * kappa() * s * s / d;
    };
    auto xZg = [&](double s){
        double d = (s - mZ * mZ) * (s - mZ * mZ) + gZ * gZ * mZ * mZ;
        return kappa() * s * (s - mZ * mZ) / d;
    };

    auto sigma = [&](double m, int q){
        double s = m * m;
        double Qq = (q == 2 ? 2. / 3. : -1. / 3.);
        double T3 = (q == 2 ? 0.5 : -0.5);
        double Vq = T3 - 2 * sw2 * Qq;
        double Aq = T3;
        double Vl = -0.5 + 2 * sw2;
        double Al = -0.5;

        double pref = 4 * M_PI * alpha * alpha / (3 * s * Nc);
        return pref * (
            Qq * Qq
            - 2 * Qq * Vl * Vq * xZg(s)
            + (Al * Al + Vl * Vl) * (Aq * Aq + Vq * Vq) * xZ(s)
        );
    };

    double val = 0.;
    val += Lqq(mass, cu) * sigma(mass, 2);
    val += Lqq(mass, cd) * sigma(mass, 1);
    val += Lqq(mass, cs) * sigma(mass, 3);

    return val > 0 ? val : 1e-12;
}

} // anonymous namespace


// ============================================================
// Constructors
// ============================================================

RooDYCoreMod::RooDYCoreMod(
    const char* name,
    const char* title,
    RooAbsReal& _m,
    const std::string& cat
) :
    RooAbsPdf(name, title),
    m("m", "m", this, _m)
{
    const DYChebModParameters parameters = loadDYCoreModParameters(cat);
    c1_ = parameters.c1;
    c2_ = parameters.c2;
    c3_ = parameters.c3;
}


RooDYCoreMod::RooDYCoreMod(
    const RooDYCoreMod& other,
    const char* name
) :
    RooAbsPdf(other, name),
    m("m", this, other.m),
    c1_(other.c1_),
    c2_(other.c2_),
    c3_(other.c3_)
{
}


// ============================================================
// evaluate
//
//   f(m) = core(m) * (1 + c1*T1(x) + c2*T2(x) + c3*T3(x))
//
//   x = (2*m-(xmin+xmax))/(xmax-xmin)
//
// The x range is the range used for the per-category MC fit
// (110 - 150 GeV), identical to the range used by
// Background/test/fitDYshapes.cpp.
// ============================================================

Double_t RooDYCoreMod::evaluate() const
{
    const double mass = m;

    constexpr double xmin = 110.0;
    constexpr double xmax = 150.0;

    const double base = dyCoreShape(mass);

    const double x = (2.0 * mass - (xmin + xmax)) / (xmax - xmin);

    const double acosx = std::acos(x);

    const double cheb =
          1.0
        + c1_ * std::cos(1.0 * acosx)
        + c2_ * std::cos(2.0 * acosx)
        + c3_ * std::cos(3.0 * acosx);

    const double value = base * cheb;

    return value > 0.0 ? value : 1e-12;
}


#include "TClass.h"

ClassImp(RooDYCoreMod)
