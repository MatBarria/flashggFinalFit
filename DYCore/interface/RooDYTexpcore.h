#ifndef FLASHGGFINALFIT_BACKGROUND_ROODYTEXPCORE_H
#define FLASHGGFINALFIT_BACKGROUND_ROODYTEXPCORE_H

#include "RooAbsPdf.h"
#include "RooRealProxy.h"
#include "Rtypes.h"
#include <string>

class RooDYTexpcore : public RooAbsPdf {

public:

    RooDYTexpcore() {}

    RooDYTexpcore(
        const char* name,
        const char* title,
        RooAbsReal& _m,
        const std::string& cat
    );

    RooDYTexpcore(
        const RooDYTexpcore& other,
        const char* name = 0
    );

    virtual TObject* clone(
        const char* newname
    ) const override
    {
        return new RooDYTexpcore(*this, newname);
    }

    inline virtual ~RooDYTexpcore() {}

protected:

    RooRealProxy m;
    double theta_;
    double a1_;
    double b1_;
    double c1_;
    double a2_;

    Double_t evaluate() const override;

public:

    ClassDef(RooDYTexpcore, 1)
};

#endif
