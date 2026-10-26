#ifndef FLASHGGFINALFIT_DYCORE_ROODYCOREMOD_H
#define FLASHGGFINALFIT_DYCORE_ROODYCOREMOD_H

#include "RooAbsPdf.h"
#include "RooRealProxy.h"
#include "Rtypes.h"

#include <string>

// =============================================================
// RooDYCoreMod
//
// Same fixed DY shape as RooDYCore (Chebyshev Lqq * EW sigma),
// multiplied by a category dependent third order Chebyshev
// correction
//
//     f(m) = RooDYCore_shape(m)
//            *
//            ( 1 + c1*T1(x) + c2*T2(x) + c3*T3(x) )
//
// with
//
//     x = (2*m - (xmin+xmax)) / (xmax-xmin)   in [-1,1]
//     xmin = 110, xmax = 150                  (MC fit range)
//
// The coefficients c1, c2, c3 are read from
//
//     DYshapes/fit_results_DYCoreMod.csv
//
// according to the category name passed to the constructor.
// That CSV is produced by Background/test/fitDYshapes.cpp.
// =============================================================

class RooDYCoreMod : public RooAbsPdf {

public:

    RooDYCoreMod() {}

    RooDYCoreMod(
        const char* name,
        const char* title,
        RooAbsReal& _m,
        const std::string& cat
    );

    RooDYCoreMod(
        const RooDYCoreMod& other,
        const char* name = 0
    );

    virtual TObject* clone(
        const char* newname
    ) const override
    {
        return new RooDYCoreMod(*this, newname);
    }

    inline virtual ~RooDYCoreMod() {}

protected:

    RooRealProxy m;

    // third order Chebyshev correction (T1, T2, T3 terms)
    double c1_;
    double c2_;
    double c3_;

    Double_t evaluate() const override;

public:

    ClassDef(RooDYCoreMod, 1)
};

#endif
