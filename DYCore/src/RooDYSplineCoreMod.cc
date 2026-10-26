#include "../interface/RooDYSplineCoreMod.h"
#include "../interface/DYCoreParameters.h"

#include "TFile.h"
#include "TSpline.h"

#include <cmath>
#include <iostream>
#include <cstdlib>
#include <string>
#include <stdexcept>
// ============================================================
// Local helper: the fixed RooDYSplineCore shape.
//
// This is a verbatim copy of the shape implemented in
// RooDYSplineCore::evaluate() (lazy loaded FEWZ spline from
// Background/DYshapes/fewz_spline.root).
//
// It is duplicated here on purpose so that the frozen "core"
// classes stay untouched.  If the fixed core is ever updated,
// remember to update both places.
// ============================================================

namespace {

double dySplineShape(double mass) {
    static TSpline3 *cached = nullptr;

    if (!cached) {

        const char *cmsswBase = std::getenv("CMSSW_BASE");

        if (!cmsswBase || !*cmsswBase) {
            throw std::runtime_error(
                "CMSSW_BASE is not set. Run cmsenv first.");
        }

        const std::string filename =
            std::string(cmsswBase) +
            "/src/flashggFinalFit/Background/DYshapes/fewz_spline.root";

        const char *splinename = "fewz_spline_smear";

        TFile *f = TFile::Open(filename.c_str());

        if (!f || f->IsZombie()) {
            std::cerr << "[ERROR] Cannot open spline file: " << filename
                      << std::endl;
            return 1e-12;
        }

        TSpline3 *tmp = (TSpline3 *)f->Get(splinename);

        if (!tmp) {
            std::cerr << "[ERROR] Cannot find spline: " << splinename
                      << std::endl;
            f->Close();
            return 1e-12;
        }

        cached = (TSpline3 *)tmp->Clone();
        cached->SetName("cached_fewz_spline_mod");

        f->Close();

        std::cout << "[INFO] FEWZ spline loaded (lazy, modified core)"
                  << std::endl;
    }

    double xmin = cached->GetXmin();
    double xmax = cached->GetXmax();

    if (mass < xmin || mass > xmax)
        return 1e-12;

    double val = cached->Eval(mass);

    return val > 0 ? val : 1e-12;
}

} // anonymous namespace

// ============================================================
// Constructors
// ============================================================

RooDYSplineCoreMod::RooDYSplineCoreMod(const char *name, const char *title,
                                       RooAbsReal &_m, const std::string &cat)
    : RooAbsPdf(name, title), m("m", "m", this, _m) {
    const DYChebModParameters parameters = loadDYSplineCoreModParameters(cat);
    c1_ = parameters.c1;
    c2_ = parameters.c2;
    c3_ = parameters.c3;
}

RooDYSplineCoreMod::RooDYSplineCoreMod(const RooDYSplineCoreMod &other,
                                       const char *name)
    : RooAbsPdf(other, name), m("m", this, other.m), c1_(other.c1_),
      c2_(other.c2_), c3_(other.c3_) {}

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

Double_t RooDYSplineCoreMod::evaluate() const {
    const double mass = m;

    constexpr double xmin = 110.0;
    constexpr double xmax = 150.0;

    const double base = dySplineShape(mass);

    const double x = (2.0 * mass - (xmin + xmax)) / (xmax - xmin);

    const double acosx = std::acos(x);

    const double cheb = 1.0 + c1_ * std::cos(1.0 * acosx) +
                        c2_ * std::cos(2.0 * acosx) +
                        c3_ * std::cos(3.0 * acosx);

    const double value = base * cheb;

    return value > 0.0 ? value : 1e-12;
}

#include "TClass.h"

ClassImp(RooDYSplineCoreMod)
