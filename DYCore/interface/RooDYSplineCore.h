#ifndef ROO_DY_SPLINE_CORE
#define ROO_DY_SPLINE_CORE

#include "RooAbsPdf.h"
#include "RooRealProxy.h"
#include "TSpline.h"

class RooDYSplineCore : public RooAbsPdf {
public:
  RooDYSplineCore() {}

  RooDYSplineCore(const char* name,
                  const char* title,
                  RooAbsReal& _m);

  RooDYSplineCore(const RooDYSplineCore& other,
                  const char* name=0);

  virtual TObject* clone(const char* newname) const override {
    return new RooDYSplineCore(*this,newname);
  }

  virtual ~RooDYSplineCore() {}

protected:
  RooRealProxy m;

  Double_t evaluate() const override;

public:
  ClassDef(RooDYSplineCore,1)
};

#endif