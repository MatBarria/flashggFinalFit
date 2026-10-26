#include "../interface/RooDYTPowcore.h"
#include "../interface/DYCoreParameters.h"

#include <cmath>


RooDYTPowcore::RooDYTPowcore(
    const char* name,
    const char* title,
    RooAbsReal& _m,
    const std::string& cat
) :
    RooAbsPdf(name, title),
    m("m", "m", this, _m)
{
    const DYPowParameters parameters = loadDYPowParameters(cat);
    frac_ = parameters.frac;
    a1_ = parameters.a1;
    b1_ = parameters.b1;
    c1_ = parameters.c1;
    a2_ = parameters.a2;
}


RooDYTPowcore::RooDYTPowcore(
    const RooDYTPowcore& other,
    const char* name
) :
    RooAbsPdf(other, name),
    m("m", this, other.m),
    frac_(other.frac_),
    a1_(other.a1_),
    b1_(other.b1_),
    c1_(other.c1_),
    a2_(other.a2_)
{
}


// ============================================================
// evaluate
//
// Same functional form as PdfModelBuilder::get2PowerLaw()
//
// x = m / 125
//
// f(m) =
//
//   frac *
//   x^-(a1 + b1*x + c1*x^2)
//
//   +
//   (1-frac) *
//   x^-a2
//
// All parameters are fixed here.
// ============================================================

Double_t RooDYTPowcore::evaluate() const
{
    const double mass = m;


    // ========================================================
    // Parameters from MC fit
    //
    // Replace these with your MC fit results.
    // ========================================================


    // ========================================================
    // Normalized mass variable
    //
    // Exactly the same as get2PowerLaw():
    //
    // x = m / 125
    //
    // 110 GeV -> 0.88
    // 125 GeV -> 1.00
    // 150 GeV -> 1.20
    // ========================================================

    const double x =
        mass / 125.0;


    // ========================================================
    // First power law
    //
    // x^-(a1 + b1*x + c1*x^2)
    // ========================================================

    const double exponent1 =
                    a1_
                + b1_ * x
                + c1_ * x * x;


    const double power1 =
        std::pow(x, -exponent1);


    // ========================================================
    // Second power law
    //
    // x^-a2
    // ========================================================

    const double power2 =
        std::pow(x, -a2_);


    // ========================================================
    // Mixture
    // ========================================================

    const double value =
                    frac_ * power1
                + (1.0 - frac_) * power2;


    // ========================================================
    // Numerical protection
    // ========================================================

    return value > 0.0 ? value : 1e-12;
}


#include "TClass.h"

ClassImp(RooDYTPowcore)
