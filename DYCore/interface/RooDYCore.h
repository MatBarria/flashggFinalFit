#ifndef FLASHGGFINALFIT_BACKGROUND_ROODYCORE_H 
#define FLASHGGFINALFIT_BACKGROUND_ROODYCORE_H 
#include "RooAbsPdf.h" 
#include "RooRealProxy.h" 
#include "Rtypes.h" 

class RooDYCore : public RooAbsPdf { 
public:
 RooDYCore() {} 
 
 RooDYCore(const char* name, const char* title, RooAbsReal& _m); 
 
 RooDYCore(const RooDYCore& other, const char* name = 0); 
 
 virtual TObject* clone(const char* newname) const 
 override { 
   return new RooDYCore(*this, newname); 
   } 
   
   inline virtual ~RooDYCore() {} 
   
   protected: RooRealProxy m; // observable 
   
   Double_t evaluate() const override; 
   
   public: 
     ClassDef(RooDYCore, 1) 
   }; 
   
#endif