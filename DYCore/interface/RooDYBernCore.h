#ifndef FLASHGGFINALFIT_BACKGROUND_ROODYBERNCORE_H
#define FLASHGGFINALFIT_BACKGROUND_ROODYBERNCORE_H

#include "RooAbsPdf.h"
#include "RooRealProxy.h"
#include "Rtypes.h"

class RooDYBernCore : public RooAbsPdf {

public:

  RooDYBernCore() {}

  RooDYBernCore(
      const char* name,
      const char* title,
      RooAbsReal& _m
  );

  RooDYBernCore(
      const RooDYBernCore& other,
      const char* name = 0
  );

  virtual TObject* clone(const char* newname) const override {
    return new RooDYBernCore(*this,newname);
  }

  inline virtual ~RooDYBernCore() {}

protected:

  RooRealProxy m;

  Double_t evaluate() const override;

public:

  ClassDef(RooDYBernCore,1)

};

#endif
