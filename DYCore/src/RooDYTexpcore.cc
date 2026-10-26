#include "../interface/RooDYTexpcore.h"
#include "../interface/DYCoreParameters.h"

#include <cmath>

RooDYTexpcore::RooDYTexpcore(
    const char* name,
    const char* title,
    RooAbsReal& _m,
    const std::string& cat
) :
    RooAbsPdf(name, title),
    m("m", "m", this, _m)
{
    const DYExpParameters parameters = loadDYExpParameters(cat);
    theta_ = parameters.theta;
    a1_ = parameters.a1;
    b1_ = parameters.b1;
    c1_ = parameters.c1;
    a2_ = parameters.a2;
}


RooDYTexpcore::RooDYTexpcore(
    const RooDYTexpcore& other,
    const char* name
) :
    RooAbsPdf(other, name),
    m("m", this, other.m),
    theta_(other.theta_),
    a1_(other.a1_),
    b1_(other.b1_),
    c1_(other.c1_),
    a2_(other.a2_)
{
}


// ============================================================
// evaluate
//
// Same functional form as PdfModelBuilder::get2Exp()
//
// x = (2*m - (xmin+xmax)) / (xmax-xmin)
//
// f(m) =
//   frac * exp(
//       a1*x
//     + b1*x^2
//     + c1*x^3
//   )
//   +
//   (1-frac) * exp(a2*x)
//
// frac = 1/(1+exp(-theta))
//
// All parameters are fixed here.
// ============================================================

Double_t RooDYTexpcore::evaluate() const
{
    const double mass = m;


    // ========================================================
    // Mass range
    //
    // IMPORTANT:
    // This MUST be identical to the range used in the MC fit.
    // ========================================================

    constexpr double xmin = 110.0;
    constexpr double xmax = 150.0;


    // ========================================================
    // Parameters from MC fit
    //
    // Replace these numbers with the parameters obtained from
    // your MC bern / 2Exp fit script.
    // ========================================================


    // ========================================================
    // Normalized observable
    //
    // Same as get2Exp():
    //
    // x = (2*m-(xmin+xmax))/(xmax-xmin)
    //
    // Therefore:
    //
    // m = 110 -> x = -1
    // m = 130 -> x =  0
    // m = 150 -> x = +1
    // ========================================================

    const double x =
        (2.0 * mass - (xmin + xmax))
        / (xmax - xmin);


    // ========================================================
    // Logistic fraction
    //
    // frac = 1/(1+exp(-theta))
    // ========================================================

    const double frac =
        1.0 / (1.0 + std::exp(-theta_));


    // ========================================================
    // First exponential
    // ========================================================

    const double exponent1 =
                    a1_ * x
                + b1_ * x * x
                + c1_ * x * x * x;


    const double exp1 =
        std::exp(exponent1);


    // ========================================================
    // Second exponential
    // ========================================================

    const double exp2 =
        std::exp(a2_ * x);


    // ========================================================
    // Mixture
    // ========================================================

    const double value =
          frac * exp1
        + (1.0 - frac) * exp2;


    // ========================================================
    // Protect against numerical problems
    // ========================================================

    return value > 0.0 ? value : 1e-12;
}


#include "TClass.h"

ClassImp(RooDYTexpcore)
