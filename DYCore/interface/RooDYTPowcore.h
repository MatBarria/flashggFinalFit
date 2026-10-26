#ifndef FLASHGGFINALFIT_BACKGROUND_ROODYTPOWCORE_H
#define FLASHGGFINALFIT_BACKGROUND_ROODYTPOWCORE_H

#include "RooAbsPdf.h"
#include "RooRealProxy.h"
#include "Rtypes.h"
#include <string>

class RooDYTPowcore : public RooAbsPdf {

public:

    RooDYTPowcore() {}

    RooDYTPowcore(
        const char* name,
        const char* title,
        RooAbsReal& _m,
        const std::string& cat
    );

    RooDYTPowcore(
        const RooDYTPowcore& other,
        const char* name = 0
    );

    virtual TObject* clone(
        const char* newname
    ) const override
    {
        return new RooDYTPowcore(*this, newname);
    }

    inline virtual ~RooDYTPowcore() {}

protected:

    RooRealProxy m;
    double frac_;
    double a1_;
    double b1_;
    double c1_;
    double a2_;

    Double_t evaluate() const override;

public:

    ClassDef(RooDYTPowcore, 1)
};

#endif
