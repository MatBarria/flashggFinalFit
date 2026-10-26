#include "TCanvas.h"

#include "RooPlot.h"
#include "RooProdPdf.h"
#include "RooBernstein.h"
#include "RooChebychev.h"
#include "RooPolynomial.h"
#include "RooGenericPdf.h"
#include "RooExponential.h"
#include "../interface/RooPowerLaw.h"
#include "../interface/RooPowerLawSum.h"
#include "../../DYCore/interface/RooDYCore.h"
#include "../../DYCore/interface/RooDYCoreMod.h"
#include "../../DYCore/interface/RooDYSplineCore.h"
#include "../../DYCore/interface/RooDYSplineCoreMod.h"
#include "../../DYCore/interface/RooDYBernCore.h"
#include "../../DYCore/interface/RooDYTexpcore.h"
#include "../../DYCore/interface/RooDYTPowcore.h"
#include "RooKeysPdf.h"
#include "RooAddPdf.h"
#include "RooDataHist.h"
#include "RooHistPdf.h"
#include "RooArgList.h"
#include "RooArgSet.h"
#include "RooConstVar.h"
#include "RooFitResult.h"
#include "RooRandom.h"
#include "RooRealVar.h"
#include "RooFormulaVar.h"
#include "TRandom3.h"

#include <algorithm>
#include <cmath>

#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/classification.hpp"
#include "boost/algorithm/string/predicate.hpp"

#include "../interface/PdfModelBuilder.h"

#include "HiggsAnalysis/CombinedLimit/interface/HGGRooPdfs.h"
#include "HiggsAnalysis/CombinedLimit/interface/RooBernsteinFast.h"

using namespace std;
using namespace RooFit;
using namespace boost;

PdfModelBuilder::PdfModelBuilder():
  obs_var_set(false),
  signal_modifier_set(false),
  signal_set(false),
  bkgHasFit(false),
  sbHasFit(false),
  keysPdfAttributesSet(false),
  verbosity(0)
{
  
  recognisedPdfTypes.push_back("Bernstein");
  recognisedPdfTypes.push_back("ModBreitWigner");
  recognisedPdfTypes.push_back("Exponential");
  recognisedPdfTypes.push_back("PowerLaw");
  recognisedPdfTypes.push_back("Laurent");
  recognisedPdfTypes.push_back("DYTheory");
  recognisedPdfTypes.push_back("KeysPdf");
  recognisedPdfTypes.push_back("File");

  wsCache = new RooWorkspace("PdfModelBuilderCache");

};

PdfModelBuilder::~PdfModelBuilder(){};

void PdfModelBuilder::setObsVar(RooRealVar *var){
  obs_var=var;
  obs_var_set=true;
}

void PdfModelBuilder::setSignalModifier(RooRealVar *var){
  signalModifier=var;
  signal_modifier_set=true;
}

void PdfModelBuilder::setSignalModifierVal(float val){
  signalModifier->setVal(val);
}

void PdfModelBuilder::setSignalModifierConstant(bool val){
  signalModifier->setConstant(val);
}

RooAbsPdf* PdfModelBuilder::getChebychev(string prefix, int order){
  
  RooArgList *coeffList = new RooArgList();
  for (int i=0; i<order; i++){
    string name = Form("%s_p%d",prefix.c_str(),i);
    //params.insert(pair<string,RooRealVar*>(name, new RooRealVar(name.c_str(),name.c_str(),1.0,0.,5.)));
    RooRealVar *param = new RooRealVar(name.c_str(),name.c_str(),0.01,-10.,10.);
    //RooFormulaVar *form = new RooFormulaVar(Form("%s_sq",name.c_str()),Form("%s_sq",name.c_str()),"@0*@0",RooArgList(*param));
    params.insert(pair<string,RooRealVar*>(name,param));
    //prods.insert(pair<string,RooFormulaVar*>(name,form));
    coeffList->add(*params[name]);
  }
  //RooChebychev *cheb = new RooChebychev(prefix.c_str(),prefix.c_str(),*obs_var,*coeffList);
  RooPolynomial *cheb = new RooPolynomial(prefix.c_str(),prefix.c_str(),*obs_var,*coeffList);
  return cheb;
  //bkgPdfs.insert(pair<string,RooAbsPdf*>(bern->GetName(),bern));

}


RooAbsPdf* PdfModelBuilder::getModBreitWigner(string prefix) {
    // fixed parameter Z mass and width
    RooRealVar* mZ = new RooRealVar(Form("%s_mZ", prefix.c_str()), "Z mass", 91.1876);
    mZ->setConstant(true);  // fixed
    
    RooRealVar* gammaZ = new RooRealVar(Form("%s_gammaZ", prefix.c_str()), "Z width", 2.4952);
    gammaZ->setConstant(true);  // fixed
    
    // floating parameters
    RooRealVar* alpha1 = new RooRealVar(Form("%s_alpha1", prefix.c_str()), "Shape exponent", 
                                        2.0,  // ini
                                        -1.0,  // min
                                        5.0); // max
    
    // floating parameters
    RooRealVar* alpha2 = new RooRealVar(Form("%s_alpha2", prefix.c_str()), "Linear exponent", 
                      0.0,   // ini
                      -0.1,  // min
                      0.1);  // max
    
    // floating parameters
    RooRealVar* alpha3 = new RooRealVar(Form("%s_alpha3", prefix.c_str()), "Quadratic exponent", 
                      0.0,     // ini
                      -0.0020,   // min
                      0.0020);  // max
    
    // store floating
    params.insert({alpha1->GetName(), alpha1});
    params.insert({alpha2->GetName(), alpha2});
    params.insert({alpha3->GetName(), alpha3});
    
    // math
    std::string formula_str = "exp(@3*(@0-125) + @4*(@0-125)*(@0-125)) / "
                             "(pow(fabs(@0 - @1), @2) + pow(@5/2.0, @2))";
    
    //PDF
    return new RooGenericPdf(prefix.c_str(), "Modified Breit-Wigner",
                             formula_str.c_str(),
                             RooArgList(*obs_var, *mZ, *alpha1, *alpha2, *alpha3, *gammaZ));
}

RooAbsPdf* PdfModelBuilder::getBWZGamma(string prefix)
{

    // ----------------------------------
    // fixed Z parameters
    // ----------------------------------

    RooRealVar* mZ =
        new RooRealVar(
            Form("%s_mZ", prefix.c_str()),
            "Z mass",
            91.1876
        );
    mZ->setConstant(true);


    RooRealVar* gammaZ =
        new RooRealVar(
            Form("%s_gammaZ", prefix.c_str()),
            "Z width",
            2.4952
        );
    gammaZ->setConstant(true);



    // ----------------------------------
    // exponential slope
    // exp(a*m)
    // ----------------------------------

    RooRealVar* a =
        new RooRealVar(
            Form("%s_a", prefix.c_str()),
            "exp slope",
            0.0,
            -0.06,
             0.06
        );



    // ----------------------------------
    // raw fraction parameter
    //
    // frac =
    // 0.5 + atan(fracRaw)/pi
    //
    // frac range:
    // (0,1)
    //
    // initial fracRaw=2 gives:
    // frac ~ 0.85
    // ----------------------------------

    RooRealVar* fracRaw =
        new RooRealVar(
            Form("%s_fracRaw", prefix.c_str()),
            "raw BW fraction",
            2.0,
            -100.0,
             100.0
        );


    params.insert({a->GetName(), a});
    params.insert({fracRaw->GetName(), fracRaw});



    // ----------------------------------
    // BWZGamma model
    //
    // frac * BWZ
    //
    // +
    //
    // (1-frac) * continuum
    //
    // ----------------------------------

    std::string formula =

        // BW fraction
        "(0.5+atan(@4)/3.141592653589793)*("

            // BWZ
            "@2*exp(@3*@0)"
            "/((@0-@1)*(@0-@1)+(@2/2.)*(@2/2.))"

        ")"

        "+"

        // continuum fraction
        "(0.5-atan(@4)/3.141592653589793)*("

            "exp(@3*@0)/(@0*@0)"

        ")";



    std::cout
        << "BWZGamma formula = "
        << formula
        << std::endl;



    return new RooGenericPdf(
        prefix.c_str(),
        "BWZGamma",
        formula.c_str(),

        RooArgList(
            *obs_var,   // @0 : mass
            *mZ,        // @1 : Z mass
            *gammaZ,    // @2 : Z width
            *a,         // @3 : exp slope
            *fracRaw    // @4 : raw fraction
        )
    );
}

RooAbsPdf* PdfModelBuilder::get2Exp(string prefix)
{

    //---------------------------------
    // normalized observable
    //---------------------------------

    double xmin = obs_var->getMin();
    double xmax = obs_var->getMax();

    string xnorm =
        Form("((2*@0-(%g+%g))/(%g-%g))",
             xmin,
             xmax,
             xmax,
             xmin);


    //---------------------------------
    // mixing fraction:  F = 1/(1+exp(-theta))
    //
    // theta >= 0 keeps F >= 0.5, i.e. the high-order exponential is always
    // the main component and the linear one only a correction.  This is what
    // the model is meant to describe.
    //---------------------------------

    RooRealVar* theta =
        new RooRealVar(
            Form("%s_theta",prefix.c_str()),
            "logistic fraction parameter (F >= 0.5)",
            1.0,
            0.0,
            5.0
        );


    //---------------------------------
    // high-order (cubic) exponential -- main component
    //
    // Its log-slope is written as a *positive* Bernstein polynomial
    //
    //   -dP1/dx = p0*(1-t)^2 + 2*p1*t*(1-t) + p2*t^2 ,   t = (1+x)/2
    //
    // so for p0,p1,p2 >= 0 this term decreases monotonically over the whole
    // range.  p0 is the |log-slope| at the low-mass edge (x=-1), p2 the one
    // at the high-mass edge (x=+1) and p1 the middle of the range.
    // Integrating the above gives
    //
    //   P1(x) = -[ q0*x + (q1/2)*x^2 + (q2/3)*x^3 ]
    //   q0 = (p0+2p1+p2)/4 , q1 = (p2-p0)/2 , q2 = (p0-2p1+p2)/4
    //---------------------------------

    RooRealVar* p0 =
        new RooRealVar(
            Form("%s_p0",prefix.c_str()),
            "log-slope at low mass",
            2.0,
            0.0,
            12.0
        );

    RooRealVar* p1 =
        new RooRealVar(
            Form("%s_p1",prefix.c_str()),
            "log-slope in the middle",
            1.5,
            0.0,
            12.0
        );

    RooRealVar* p2 =
        new RooRealVar(
            Form("%s_p2",prefix.c_str()),
            "log-slope at high mass",
            1.0,
            0.0,
            12.0
        );


    //---------------------------------
    // low-order (linear) exponential -- correction only
    //
    // a2 <= 0 keeps this term falling as well, so the sum of the two terms
    // can never develop the "W" shape produced by two mis-aligned
    // exponentials (which is what happened when a2 was allowed to be > 0).
    //---------------------------------

    RooRealVar* a2 =
        new RooRealVar(
            Form("%s_a2",prefix.c_str()),
            "linear2 (correction)",
            -1.0,
            -5.0,
            0.0
        );


    //---------------------------------
    // save parameters
    //---------------------------------

    params.insert({theta->GetName(),theta});

    params.insert({p0->GetName(),p0});
    params.insert({p1->GetName(),p1});
    params.insert({p2->GetName(),p2});

    params.insert({a2->GetName(),a2});


    //---------------------------------
    // logistic fraction
    //---------------------------------

    string frac =
        "(1/(1+exp(-@1)))";


    //---------------------------------
    // main (high-order) exponential -- monotone by construction
    //---------------------------------

    string mainExp =
        "exp(-(((@2+2*@3+@4)/4)*" + xnorm
        + "+((@4-@2)/4)*pow(" + xnorm + ",2)"
        + "+((@2-2*@3+@4)/12)*pow(" + xnorm + ",3)))";


    //---------------------------------
    // small falling correction
    //---------------------------------

    string corrExp =
        "exp(@5*" + xnorm + ")";


    //---------------------------------
    // construct formula
    //---------------------------------

    string formula =
        frac + "*" + mainExp
        + "+(1-" + frac + ")*" + corrExp;


    //---------------------------------
    // return pdf
    //---------------------------------

    return new RooGenericPdf(
        prefix.c_str(),
        "TwoExponentials (high-order main term + falling correction)",
        formula.c_str(),
        RooArgList(
            *obs_var,
            *theta,
            *p0,
            *p1,
            *p2,
            *a2
        )
    );

}

namespace {
  // small helper: does 'name' end with 'suffix'?
  bool hasSuffix(const string &name, const string &suffix){
    return name.size() > suffix.size() &&
           name.compare(name.size()-suffix.size(), suffix.size(), suffix) == 0;
  }
}

// ---------------------------------------------------------------------------
// Produce a new starting configuration for the sum-of-exponentials pdf.
//
// The pdf is monotone by construction (see get2Exp):
//    f(x) = F*exp(P1(x)) + (1-F)*exp(a2*x)
// with -dP1/dx a positive Bernstein polynomial (p0,p1,p2 >= 0) and a2 <= 0.
// Only the scale of the log-slopes and the admixture F have to be varied, so
// the starting points simply draw those parameters inside their ranges.
//
// The random numbers are seeded from the pdf name and 'attempt', so every
// attempt gives a different (but reproducible) starting point.
// ---------------------------------------------------------------------------
bool PdfModelBuilder::set2ExpMonotonicInitValues(RooAbsPdf *pdf, unsigned int attempt)
{
  if (!pdf) return false;

  // locate the 2Exp parameters by their names:
  //   <prefix>_theta, <prefix>_p0, <prefix>_p1, <prefix>_p2, <prefix>_a2
  RooRealVar *theta = 0;
  RooRealVar *p0    = 0;
  RooRealVar *p1    = 0;
  RooRealVar *p2    = 0;
  RooRealVar *a2    = 0;

  RooArgSet *vars = pdf->getVariables();
  TIterator *varIt = vars->createIterator();
  TObject *obj = 0;
  while ((obj = varIt->Next())) {
    RooRealVar *var = dynamic_cast<RooRealVar*>(obj);
    if (!var) continue;
    const string name = var->GetName();
    if      (hasSuffix(name, "_theta")) theta = var;
    else if (hasSuffix(name, "_p0"))    p0    = var;
    else if (hasSuffix(name, "_p1"))    p1    = var;
    else if (hasSuffix(name, "_p2"))    p2    = var;
    else if (hasSuffix(name, "_a2"))    a2    = var;
  }
  delete varIt;
  delete vars;

  // not a 2Exp pdf -> nothing to do
  if (!theta || !p0 || !p1 || !p2 || !a2) return false;

  // deterministic seed, different for every (pdf, attempt) pair
  ULong64_t seed = 1469598103934665603ULL;
  for (const char *ch = pdf->GetName(); *ch != '\0'; ++ch) {
    seed *= 1099511628211ULL;
    seed ^= static_cast<ULong64_t>(static_cast<unsigned char>(*ch));
  }
  seed += 0x9E3779B97F4A7C15ULL * (static_cast<ULong64_t>(attempt) + 1ULL);

  TRandom3 rnd(seed);

  // draw a value inside [lo,hi] and clip it to the parameter range
  auto drawVal = [&](RooRealVar *var, double lo, double hi) {
    const double v = rnd.Uniform(lo, hi);
    var->setVal(TMath::Max(var->getMin(), TMath::Min(var->getMax(), v)));
  };

  // log-slopes: log-uniform so that both shallow and steep shapes are probed
  auto drawLog = [&](RooRealVar *var, double lo, double hi) {
    const double v = std::exp(rnd.Uniform(std::log(lo), std::log(hi)));
    var->setVal(TMath::Max(var->getMin(), TMath::Min(var->getMax(), v)));
  };

  drawLog(p0, 0.3, 5.0);
  drawLog(p1, 0.3, 5.0);
  drawLog(p2, 0.3, 5.0);

  // fraction: always dominated by the high-order term (F >= 0.5)
  drawVal(theta, 0.0, 3.0);

  // small falling correction
  drawVal(a2, -3.0, 0.0);

  cout << "[INFO] 2Exp start " << attempt << " for " << pdf->GetName()
       << " : theta = " << theta->getVal()
       << ", p0 = "    << p0->getVal()
       << ", p1 = "    << p1->getVal()
       << ", p2 = "    << p2->getVal()
       << ", a2 = "    << a2->getVal() << endl;

  return true;
}

RooAbsPdf* PdfModelBuilder::get2PowerLaw(string prefix)
{

    // ------------------------------------
    // mixture fraction
    // ------------------------------------

    RooRealVar* frac =
        new RooRealVar(
            Form("%s_frac",prefix.c_str()),
            "fraction",
            0.95,
            0.5,
            1.0
        );


    // ------------------------------------
    // first power law
    //
    // x^-(a1+b1*x+c1*x^2)
    // ------------------------------------

    RooRealVar* a1 =
        new RooRealVar(
            Form("%s_a1",prefix.c_str()),
            "const1",
            2.5,
            0.0,
            25.0
        );


    RooRealVar* b1 =
        new RooRealVar(
            Form("%s_b1",prefix.c_str()),
            "linear1",
            0.0,
            -15.0,
            15.0
        );


    RooRealVar* c1 =
        new RooRealVar(
            Form("%s_c1",prefix.c_str()),
            "quad1",
            0.0,
            -15.0,
            15.0
        );


    // ------------------------------------
    // second power law
    //
    // x^-a2
    // ------------------------------------

    RooRealVar* a2 =
        new RooRealVar(
            Form("%s_a2",prefix.c_str()),
            "const2",
            2.0,
            0.0,
            25.0
        );



    params.insert({frac->GetName(),frac});

    params.insert({a1->GetName(),a1});
    params.insert({b1->GetName(),b1});
    params.insert({c1->GetName(),c1});

    params.insert({a2->GetName(),a2});



    // ------------------------------------
    // normalized mass variable
    //
    // x = m / 125
    //
    // 110 GeV -> 0.88
    // 125 GeV -> 1.00
    // 150 GeV -> 1.20
    //
    // ------------------------------------

    std::string x =
        "(@0/125.0)";



    std::string formula =

        "@1*pow("
        + x +
        ",-(@2+@3*"
        + x +
        "+@4*"
        + x +
        "*"
        + x +
        "))"

        "+"

        "(1-@1)*pow("
        + x +
        ",-@5)";



    std::cout
        << "ThreePowerPlusOne formula = "
        << formula
        << std::endl;



    return new RooGenericPdf(
        prefix.c_str(),
        "ThreePowerPlusOne",
        formula.c_str(),

        RooArgList(
            *obs_var, // @0
            *frac,    // @1

            *a1,      // @2
            *b1,      // @3
            *c1,      // @4

            *a2       // @5
        )
    );
}

RooAbsPdf* PdfModelBuilder::getBernstein(string prefix, int order){
  
  RooArgList *coeffList = new RooArgList();
  //coeffList->add(RooConst(1.0)); // no need for cnstant in this interface
  for (int i=0; i<order; i++){
    string name = Form("%s_p%d",prefix.c_str(),i);
    //params.insert(pair<string,RooRealVar*>(name, new RooRealVar(name.c_str(),name.c_str(),1.0,0.,5.)));
    RooRealVar *param = new RooRealVar(name.c_str(),name.c_str(),0.1*(i+1),-15.,15.);
    RooFormulaVar *form = new RooFormulaVar(Form("%s_sq",name.c_str()),Form("%s_sq",name.c_str()),"@0*@0",RooArgList(*param));
    params.insert(pair<string,RooRealVar*>(name,param));
    prods.insert(pair<string,RooFormulaVar*>(name,form));
    coeffList->add(*prods[name]);
  }
  //RooBernstein *bern = new RooBernstein(prefix.c_str(),prefix.c_str(),*obs_var,*coeffList);
  if (order==1) {
	RooBernsteinFast<1> *bern = new RooBernsteinFast<1>(prefix.c_str(),prefix.c_str(),*obs_var,*coeffList);
  	return bern;
  } else if (order==2) {
	RooBernsteinFast<2> *bern = new RooBernsteinFast<2>(prefix.c_str(),prefix.c_str(),*obs_var,*coeffList);
  	return bern;
  } else if (order==3) {
	RooBernsteinFast<3> *bern = new RooBernsteinFast<3>(prefix.c_str(),prefix.c_str(),*obs_var,*coeffList);
  	return bern;
  } else if (order==4) {
	RooBernsteinFast<4> *bern = new RooBernsteinFast<4>(prefix.c_str(),prefix.c_str(),*obs_var,*coeffList);
  	return bern;
  } else if (order==5) {
	RooBernsteinFast<5> *bern = new RooBernsteinFast<5>(prefix.c_str(),prefix.c_str(),*obs_var,*coeffList);
  	return bern;
//  } else if (order==6) {
//	RooBernsteinFast<6> *bern = new RooBernsteinFast<6>(prefix.c_str(),prefix.c_str(),*obs_var,*coeffList);
//  	return bern;
//  } else if (order==7) {
//	RooBernsteinFast<7> *bern = new RooBernsteinFast<7>(prefix.c_str(),prefix.c_str(),*obs_var,*coeffList);
 // 	return bern;
  } else {
	return NULL;
  }
  //return bern;
  //bkgPdfs.insert(pair<string,RooAbsPdf*>(bern->GetName(),bern));

}

RooAbsPdf* PdfModelBuilder::getPowerLawGeneric(string prefix, int order){
  
  if (order%2==0){
    cerr << "ERROR -- addPowerLaw -- only odd number of params allowed" << endl;
    return NULL;
  }
  else {
    int nfracs=(order-1)/2;
    int npows=order-nfracs;
    assert(nfracs==npows-1);
    string formula="";
    RooArgList *dependents = new RooArgList();
    dependents->add(*obs_var);
    // first do recursive fraction
    if (order>1) {
      formula += "(1.-";
      for (int i=1; i<=nfracs; i++){
        if (i<nfracs) formula += Form("@%d-",i);
        else formula += Form("@%d)*",i);
        string name =  Form("%s_f%d",prefix.c_str(),i);
        params.insert(pair<string,RooRealVar*>(name, new RooRealVar(name.c_str(),name.c_str(),0.1,0.,1.)));
        dependents->add(*params[name]);
      }
    }
    for (int i=1; i<=npows; i++){
      string pname =  Form("%s_p%d",prefix.c_str(),i);
      string fname =  Form("%s_f%d",prefix.c_str(),i-1);
      params.insert(pair<string,RooRealVar*>(pname, new RooRealVar(pname.c_str(),pname.c_str(),TMath::Max(-10.,-2.*(i+1)),-10.,0.)));
      if (i==1) {
        formula += Form("TMath::Power(@0,@%d)",nfracs+i);
        dependents->add(*params[pname]);
      }
      else {
        formula += Form(" + @%d*TMath::Power(@0,@%d)",i-1,nfracs+i);
        dependents->add(*params[pname]);
      }
    }
    cout << "FORM -- " << formula << endl;
    dependents->Print("v");
    RooGenericPdf *pow = new RooGenericPdf(prefix.c_str(),prefix.c_str(),formula.c_str(),*dependents);
    pow->Print("v");
    return pow;
    //bkgPdfs.insert(pair<string,RooAbsPdf*>(pow->GetName(),pow));

  }
}

RooAbsPdf* PdfModelBuilder::getPowerLaw(string prefix, int order){
  
  RooArgList coefList;
  for (int i=0; i<order; i++){
    double start=-2.;
    double low=-10.;
    double high=0.;
    if (order>0){
      start=-0.001/double(i);
      low=-0.01;
      high=0.01;
    }
    RooRealVar *var = new RooRealVar(Form("%s_p%d",prefix.c_str(),i),Form("%s_p%d",prefix.c_str(),i),start,low,high);
    coefList.add(*var);
  }
  RooPowerLawSum *pow = new RooPowerLawSum(prefix.c_str(),prefix.c_str(),*obs_var,coefList);
  return pow;
  //bkgPdfs.insert(pair<string,RooAbsPdf*>(pow->GetName(),pow));

}

RooAbsPdf* PdfModelBuilder::getExponential(string prefix, int order){
  
  RooArgList coefList;
  for (int i=0; i<order; i++){
    double start=-1.;
    double low=-2.;
    double high=0.;
    if (order>0){
      start=-0.001/double(i);
      low=-0.01;
      high=0.01;
    }
    RooRealVar *var = new RooRealVar(Form("%s_p%d",prefix.c_str(),i),Form("%s_p%d",prefix.c_str(),i),start,low,high);
    coefList.add(*var);
  }
  RooPowerLawSum *exp = new RooPowerLawSum(prefix.c_str(),prefix.c_str(),*obs_var,coefList);
  return exp;
  //bkgPdfs.insert(pair<string,RooAbsPdf*>(exp->GetName(),exp));

}

RooAbsPdf* PdfModelBuilder::getPowerLawSingle(string prefix, int order){
  
  if (order%2==0){
    cerr << "ERROR -- addPowerLaw -- only odd number of params allowed" << endl;
    return NULL;
  }
  else {
    int nfracs=(order-1)/2;
    int npows=order-nfracs;
    assert(nfracs==npows-1);
    RooArgList *fracs = new RooArgList();
    RooArgList *pows = new RooArgList();
    for (int i=1; i<=nfracs; i++){
      string name =  Form("%s_f%d",prefix.c_str(),i);
      params.insert(pair<string,RooRealVar*>(name, new RooRealVar(name.c_str(),name.c_str(),0.9-float(i-1)*1./nfracs,0.,1.)));
      //params[name]->removeRange();
      fracs->add(*params[name]);
    }
    for (int i=1; i<=npows; i++){
      string name =  Form("%s_p%d",prefix.c_str(),i);
      string ename =  Form("%s_e%d",prefix.c_str(),i);
      params.insert(pair<string,RooRealVar*>(name, new RooRealVar(name.c_str(),name.c_str(),TMath::Max(-9.,-1.*(i+1)),-9.,1.)));
      //params[name]->removeRange();
      utilities.insert(pair<string,RooAbsPdf*>(ename, new RooPower(ename.c_str(),ename.c_str(),*obs_var,*params[name])));
      pows->add(*utilities[ename]);
    }
    //cout << "RooArgLists..." << endl;
    //fracs->Print("v");
    //pows->Print("v");
    //cout << "Function..." << endl;
    RooAbsPdf *pow = new RooAddPdf(prefix.c_str(),prefix.c_str(),*pows,*fracs,true); 
    //pow->Print("v");
    return pow;
    //bkgPdfs.insert(pair<string,RooAbsPdf*>(pow->GetName(),pow));
  }
}

RooAbsPdf* PdfModelBuilder::getLaurentSeries(string prefix, int order){
 
  int nlower=int(ceil(order/2.));
  int nhigher=order-nlower;
  // first do 0th order
  RooArgList *pows = new RooArgList();
  RooArgList *plist = new RooArgList();
  string pname =  Form("%s_pow0",prefix.c_str());
  utilities.insert(pair<string,RooAbsPdf*>(pname, new RooPower(pname.c_str(),pname.c_str(),*obs_var,RooConst(-4.))));
  pows->add(*utilities[pname]);

  // even terms
  for (int i=1; i<=nlower; i++){
    string name = Form("%s_l%d",prefix.c_str(),i);
    params.insert(pair<string,RooRealVar*>(name, new RooRealVar(name.c_str(),name.c_str(),0.25/order,0.000001,0.999999)));
    plist->add(*params[name]);
    string pname =  Form("%s_powl%d",prefix.c_str(),i);
    utilities.insert(pair<string,RooAbsPdf*>(pname, new RooPower(pname.c_str(),pname.c_str(),*obs_var,RooConst(-4.-i))));
    pows->add(*utilities[pname]);
  }
  // odd terms
  for (int i=1; i<=nhigher; i++){
    string name = Form("%s_h%d",prefix.c_str(),i);
    params.insert(pair<string,RooRealVar*>(name, new RooRealVar(name.c_str(),name.c_str(),0.25/order,0.000001,0.999999)));
    plist->add(*params[name]);
    string pname =  Form("%s_powh%d",prefix.c_str(),i);
    utilities.insert(pair<string,RooAbsPdf*>(pname, new RooPower(pname.c_str(),pname.c_str(),*obs_var,RooConst(-4.+i))));
    pows->add(*utilities[pname]);
  }
  RooAddPdf *pdf = new RooAddPdf(prefix.c_str(),prefix.c_str(),*pows,*plist,true);
  return pdf;
  //bkgPdfs.insert(pair<string,RooAbsPdf*>(pdf->GetName(),pdf));
}

/*
RooAbsPdf* PdfModelBuilder::getDYTheory(string prefix, int empOrder)
{
  // --- DY core (already a normalized RooAbsPdf) ---
  RooAbsPdf* dyCorePdf =
    new RooDYCore(Form("%s_core", prefix.c_str()),
                  "DY core",
                  *obs_var);
 
//    RooAbsPdf* dyCorePdf =
//      new RooDYSplineCore(
//        Form("%s_core", prefix.c_str()),
//        "DY spline core",
//       *obs_var
//      );

  // --- argument list ---
  RooArgList args;
  args.add(*dyCorePdf);  // @0 = DYCore(m)
  args.add(*obs_var);    // @1 = m

  // --- mhat in [-1,1] ---
  const double mMin = obs_var->getMin();
  const double mMax = obs_var->getMax();
  string mhat =
    Form("(2*@1-(%g+%g))/(%g-%g)", mMax, mMin, mMax, mMin);

  // --- build empirical correction ---
  string formula = "@0*exp(";

  for(int i = 1; i <= empOrder; i++){
    string name = Form("%s_a%d", prefix.c_str(), i);

    RooRealVar* ai =
      new RooRealVar(name.c_str(), name.c_str(), 0., -1., 1.);
    params.insert({name, ai});
    args.add(*ai);   // parameters start from @2

    if(i > 1) formula += "+";

    formula += Form("@%d*cos(%d*acos(%s))",
                    args.getSize() - 1,
                    i,
                    mhat.c_str());
  }

  formula += ")";

  return new RooGenericPdf(
    prefix.c_str(),
    "DY theory empirical",
    formula.c_str(),
    args
  );
}
*/

RooAbsPdf* PdfModelBuilder::getDYTheory(string prefix, int empOrder)
{
  // -------------------------------
  // DY core
  // -------------------------------
  RooAbsPdf* dyCorePdf =
    new RooDYCore(Form("%s_core", prefix.c_str()),
                  "DY core",
                  *obs_var);

//  RooAbsPdf* dyCorePdf =
//    new RooDYBernCore(
//        Form("%s_core", prefix.c_str()),
//        "Bernstein core",
//        *obs_var);
                  
//    RooAbsPdf* dyCorePdf =
//      new RooDYSplineCore(
//        Form("%s_core", prefix.c_str()),
//        "DY spline core",
//       *obs_var
//      );

  RooArgList args;
  args.add(*dyCorePdf);   // @0
  args.add(*obs_var);     // @1

  // map m -> [-1,1]
  const double mMin = obs_var->getMin();
  const double mMax = obs_var->getMax();

  string mhat =
      Form("(2*@1-(%g+%g))/(%g-%g)",
           mMax, mMin,
           mMax, mMin);

  constexpr double scale = 0.50;

  string formula = Form("@0*(1+%.3f*(", scale);

  for(int i=1;i<=empOrder;i++)
  {
      string name = Form("%s_a%d", prefix.c_str(), i);

      RooRealVar* ai =
          new RooRealVar(name.c_str(),
                         name.c_str(),
                         0.0,
                         -1.0,
                          1.0);

      params.insert({name, ai});
      args.add(*ai);

      if(i>1)
          formula += " + ";

      formula += Form("@%d*cos(%d*acos(%s))",
                      args.getSize()-1,
                      i,
                      mhat.c_str());
  }

  formula += "))";

  return new RooGenericPdf(
      prefix.c_str(),
      "DY theory empirical",
      formula.c_str(),
      args);
}

RooAbsPdf* PdfModelBuilder::getDYBernChebychev(string prefix, int empOrder)
{
  // -------------------------------
  // DY core
  // -------------------------------
//  RooAbsPdf* dyCorePdf =
//    new RooDYCore(Form("%s_core", prefix.c_str()),
//                  "DY core",
//                  *obs_var);

  RooAbsPdf* dyCorePdf =
    new RooDYBernCore(
        Form("%s_core", prefix.c_str()),
        "Bernstein core",
        *obs_var);
                  
//    RooAbsPdf* dyCorePdf =
//      new RooDYSplineCore(
//        Form("%s_core", prefix.c_str()),
//        "DY spline core",
//       *obs_var
//      );

  RooArgList args;
  args.add(*dyCorePdf);   // @0
  args.add(*obs_var);     // @1

  // map m -> [-1,1]
  const double mMin = obs_var->getMin();
  const double mMax = obs_var->getMax();

  string mhat =
      Form("(2*@1-(%g+%g))/(%g-%g)",
           mMax, mMin,
           mMax, mMin);

  constexpr double scale = 0.50;

  string formula = Form("@0*(1+%.3f*(", scale);

  for(int i=1;i<=empOrder;i++)
  {
      string name = Form("%s_a%d", prefix.c_str(), i);

      RooRealVar* ai =
          new RooRealVar(name.c_str(),
                         name.c_str(),
                         0.0,
                         -1.0,
                          1.0);

      params.insert({name, ai});
      args.add(*ai);

      if(i>1)
          formula += " + ";

      formula += Form("@%d*cos(%d*acos(%s))",
                      args.getSize()-1,
                      i,
                      mhat.c_str());
  }

  formula += "))";

  return new RooGenericPdf(
      prefix.c_str(),
      "DY theory empirical",
      formula.c_str(),
      args);
}

RooAbsPdf* PdfModelBuilder::getDYExpChebychev(string prefix, int empOrder, string cat)
{
  // -------------------------------
  // DY core
  // -------------------------------
//  RooAbsPdf* dyCorePdf =
//    new RooDYCore(Form("%s_core", prefix.c_str()),
//                  "DY core",
//                  *obs_var);

  RooAbsPdf* dyCorePdf =
    new RooDYTexpcore(
        Form("%s_core", prefix.c_str()),
        "exp core",
        *obs_var, cat);
                  
//    RooAbsPdf* dyCorePdf =
//      new RooDYSplineCore(
//        Form("%s_core", prefix.c_str()),
//        "DY spline core",
//       *obs_var
//      );

  RooArgList args;
  args.add(*dyCorePdf);   // @0
  args.add(*obs_var);     // @1

  // map m -> [-1,1]
  const double mMin = obs_var->getMin();
  const double mMax = obs_var->getMax();

  string mhat =
      Form("(2*@1-(%g+%g))/(%g-%g)",
           mMax, mMin,
           mMax, mMin);

  constexpr double scale = 0.50;

  string formula = Form("@0*(1+%.3f*(", scale);

  for(int i=1;i<=empOrder;i++)
  {
      string name = Form("%s_a%d", prefix.c_str(), i);

      RooRealVar* ai =
          new RooRealVar(name.c_str(),
                         name.c_str(),
                         0.0,
                         -1.0,
                          1.0);

      params.insert({name, ai});
      args.add(*ai);

      if(i>1)
          formula += " + ";

      formula += Form("@%d*cos(%d*acos(%s))",
                      args.getSize()-1,
                      i,
                      mhat.c_str());
  }

  formula += "))";

  return new RooGenericPdf(
      prefix.c_str(),
      "DY theory empirical",
      formula.c_str(),
      args);
}

RooAbsPdf* PdfModelBuilder::getDYPowChebychev(string prefix, int empOrder, string cat)
{
  // -------------------------------
  // DY core
  // -------------------------------
//  RooAbsPdf* dyCorePdf =
//    new RooDYCore(Form("%s_core", prefix.c_str()),
//                  "DY core",
//                  *obs_var);

  RooAbsPdf* dyCorePdf =
    new RooDYTPowcore(
        Form("%s_core", prefix.c_str()),
        "pow core",
        *obs_var, cat);
                  
//    RooAbsPdf* dyCorePdf =
//      new RooDYSplineCore(
//        Form("%s_core", prefix.c_str()),
//        "DY spline core",
//       *obs_var
//      );

  RooArgList args;
  args.add(*dyCorePdf);   // @0
  args.add(*obs_var);     // @1

  // map m -> [-1,1]
  const double mMin = obs_var->getMin();
  const double mMax = obs_var->getMax();

  string mhat =
      Form("(2*@1-(%g+%g))/(%g-%g)",
           mMax, mMin,
           mMax, mMin);

  constexpr double scale = 0.50;

  string formula = Form("@0*(1+%.3f*(", scale);

  for(int i=1;i<=empOrder;i++)
  {
      string name = Form("%s_a%d", prefix.c_str(), i);

      RooRealVar* ai =
          new RooRealVar(name.c_str(),
                         name.c_str(),
                         0.0,
                         -2.0,
                          2.0);

      params.insert({name, ai});
      args.add(*ai);

      if(i>1)
          formula += " + ";

      formula += Form("@%d*cos(%d*acos(%s))",
                      args.getSize()-1,
                      i,
                      mhat.c_str());
  }

  formula += "))";

  return new RooGenericPdf(
      prefix.c_str(),
      "DY theory empirical",
      formula.c_str(),
      args);
}

RooAbsPdf* PdfModelBuilder::getDYFewzChebychev(string prefix, int empOrder)
{
  // -------------------------------
  // DY core
  // -------------------------------
//  RooAbsPdf* dyCorePdf =
//    new RooDYCore(Form("%s_core", prefix.c_str()),
//                  "DY core",
//                  *obs_var);

//  RooAbsPdf* dyCorePdf =
//    new RooDYBernCore(
//        Form("%s_core", prefix.c_str()),
//        "Bernstein core",
//        *obs_var);
                  
    RooAbsPdf* dyCorePdf =
      new RooDYSplineCore(
        Form("%s_core", prefix.c_str()),
        "DY spline core",
       *obs_var
      );

  RooArgList args;
  args.add(*dyCorePdf);   // @0
  args.add(*obs_var);     // @1

  // map m -> [-1,1]
  const double mMin = obs_var->getMin();
  const double mMax = obs_var->getMax();

  string mhat =
      Form("(2*@1-(%g+%g))/(%g-%g)",
           mMax, mMin,
           mMax, mMin);

  constexpr double scale = 0.50;

  string formula = Form("@0*(1+%.3f*(", scale);

  for(int i=1;i<=empOrder;i++)
  {
      string name = Form("%s_a%d", prefix.c_str(), i);

      RooRealVar* ai =
          new RooRealVar(name.c_str(),
                         name.c_str(),
                         0.0,
                         -1.0,
                          1.0);

      params.insert({name, ai});
      args.add(*ai);

      if(i>1)
          formula += " + ";

      formula += Form("@%d*cos(%d*acos(%s))",
                      args.getSize()-1,
                      i,
                      mhat.c_str());
  }

  formula += "))";

  return new RooGenericPdf(
      prefix.c_str(),
      "DY theory empirical",
      formula.c_str(),
      args);
}

/*
RooAbsPdf* PdfModelBuilder::getDYBernstein(string prefix, int order)
{
  // --- DY core (already a normalized RooAbsPdf) ---
  RooAbsPdf* dyCorePdf =
    new RooDYCore(Form("%s_core", prefix.c_str()),
                  "DY core",
                  *obs_var);
 
//    RooAbsPdf* dyCorePdf =
//      new RooDYSplineCore(
//        Form("%s_core", prefix.c_str()),
//        "DY spline core",
//       *obs_var
//      );

  RooArgList args;

  args.add(*dyCorePdf); // @0
  args.add(*obs_var);   // @1

  string x =
    Form("(@1 - %g)/(%g - %g)",
         obs_var->getMin(),
         obs_var->getMax(),
         obs_var->getMin());

  // exp(Bernstein)
  string formula = "@0*exp(";

  for(int i = 0; i < order; i++)
  {
    string name = Form("%s_p%d", prefix.c_str(), i);

    RooRealVar* pi =
      new RooRealVar(name.c_str(),
                     name.c_str(),
                     0.0, -0.5, 0.5);

    params.insert({name, pi});
    args.add(*pi);

    if(i > 0) formula += " + ";

    int binom = TMath::Binomial(order-1, i);

    formula += Form(
      "%d*@%d*pow(%s,%d)*pow(1-%s,%d)",
      binom,
      args.getSize()-1,
      x.c_str(), i,
      x.c_str(), order-1-i
    );
  }

  formula += ")";

  return new RooGenericPdf(
    prefix.c_str(),
    "DYCore * exp(Bernstein)",
    formula.c_str(),
    args
  );
}
*/

RooAbsPdf* PdfModelBuilder::getDYBernstein(string prefix, int order)
{
  // -------------------------------
  // DY core
  // -------------------------------
  RooAbsPdf* dyCorePdf =
    new RooDYCore(Form("%s_core", prefix.c_str()),
                  "DY core",
                  *obs_var);

//  RooAbsPdf* dyCorePdf =
//    new RooDYBernCore(
//        Form("%s_core", prefix.c_str()),
//        "Bernstein core",
//        *obs_var);
          
//    RooAbsPdf* dyCorePdf =
//      new RooDYSplineCore(
//        Form("%s_core", prefix.c_str()),
//        "DY spline core",
//       *obs_var
//      );

  RooArgList args;

  args.add(*dyCorePdf);   // @0
  args.add(*obs_var);     // @1

  // x in [0,1]
  string x = Form("(@1-%g)/(%g-%g)",
                  obs_var->getMin(),
                  obs_var->getMax(),
                  obs_var->getMin());

  // strength of correction
  constexpr double scale = 0.5;

  string formula = Form("@0*(1+%.3f*(", scale);

  for (int i = 0; i < order; ++i)
  {
    string name = Form("%s_p%d", prefix.c_str(), i);

    RooRealVar* pi =
      new RooRealVar(name.c_str(),
                     name.c_str(),
                     0.0,
                     -2.0,
                      2.0);

    params.insert({name, pi});
    args.add(*pi);

    if (i > 0)
      formula += " + ";

    int binom = TMath::Binomial(order - 1, i);

    formula += Form("%d*@%d*pow(%s,%d)*pow(1-%s,%d)",
                    binom,
                    args.getSize()-1,
                    x.c_str(), i,
                    x.c_str(), order-1-i);
  }

  formula += "))";

  return new RooGenericPdf(
      prefix.c_str(),
      "DYCore * (1 + Bernstein)",
      formula.c_str(),
      args);
}

RooAbsPdf* PdfModelBuilder::getDYBernBernstein(string prefix, int order)
{
  // -------------------------------
  // DY core
  // -------------------------------
//  RooAbsPdf* dyCorePdf =
//    new RooDYCore(Form("%s_core", prefix.c_str()),
//                  "DY core",
//                  *obs_var);

  RooAbsPdf* dyCorePdf =
    new RooDYBernCore(
        Form("%s_core", prefix.c_str()),
        "Bernstein core",
        *obs_var);
          
//    RooAbsPdf* dyCorePdf =
//      new RooDYSplineCore(
//        Form("%s_core", prefix.c_str()),
//        "DY spline core",
//       *obs_var
//      );

  RooArgList args;

  args.add(*dyCorePdf);   // @0
  args.add(*obs_var);     // @1

  // x in [0,1]
  string x = Form("(@1-%g)/(%g-%g)",
                  obs_var->getMin(),
                  obs_var->getMax(),
                  obs_var->getMin());

  // strength of correction
  constexpr double scale = 0.5;

  string formula = Form("@0*(1+%.3f*(", scale);

  for (int i = 0; i < order; ++i)
  {
    string name = Form("%s_p%d", prefix.c_str(), i);

    RooRealVar* pi =
      new RooRealVar(name.c_str(),
                     name.c_str(),
                     0.0,
                     -2.0,
                      2.0);

    params.insert({name, pi});
    args.add(*pi);

    if (i > 0)
      formula += " + ";

    int binom = TMath::Binomial(order - 1, i);

    formula += Form("%d*@%d*pow(%s,%d)*pow(1-%s,%d)",
                    binom,
                    args.getSize()-1,
                    x.c_str(), i,
                    x.c_str(), order-1-i);
  }

  formula += "))";

  return new RooGenericPdf(
      prefix.c_str(),
      "DYCore * (1 + Bernstein)",
      formula.c_str(),
      args);
}

RooAbsPdf* PdfModelBuilder::getDYPowerLaw(string prefix, int order)
{
  // -------------------------------
  // DY core
  // -------------------------------
  RooAbsPdf* dyCorePdf =
    new RooDYCore(Form("%s_core", prefix.c_str()),
                  "DY core",
                  *obs_var);

//  RooAbsPdf* dyCorePdf =
//    new RooDYBernCore(
//        Form("%s_core", prefix.c_str()),
//        "Bernstein core",
//        *obs_var);
                 
//    RooAbsPdf* dyCorePdf =
//      new RooDYSplineCore(
//        Form("%s_core", prefix.c_str()),
//        "DY spline core",
//       *obs_var
//      );

  RooArgList args;
  args.add(*dyCorePdf);
  args.add(*obs_var);

  double mMin = obs_var->getMin();
  double mMax = obs_var->getMax();

  string mhat =
    Form("(2*@1-(%g+%g))/(%g-%g)", mMax, mMin, mMax, mMin);

  string formula = "@0*exp(";

  for(int i = 0; i < order; i++)
  {
    string name = Form("%s_a%d", prefix.c_str(), i);

    RooRealVar* ai =
      new RooRealVar(name.c_str(), name.c_str(),
                     0.0, -0.5, 0.5);

    params.insert({name, ai});
    args.add(*ai);

    if(i > 0) formula += " + ";

    formula += Form("@%d*pow(%s,%d)",
                    args.getSize()-1,
                    mhat.c_str(),
                    i+1);
  }

  formula += ")";

  return new RooGenericPdf(
    prefix.c_str(),
    "DYCore * exp powerlaw correction",
    formula.c_str(),
    args
  );
}

RooAbsPdf* PdfModelBuilder::getDYBernPowerLaw(string prefix, int order)
{
  // -------------------------------
  // DY core
  // -------------------------------
//  RooAbsPdf* dyCorePdf =
//    new RooDYCore(Form("%s_core", prefix.c_str()),
//                  "DY core",
//                  *obs_var);

  RooAbsPdf* dyCorePdf =
    new RooDYBernCore(
        Form("%s_core", prefix.c_str()),
        "Bernstein core",
        *obs_var);
                 
//    RooAbsPdf* dyCorePdf =
//      new RooDYSplineCore(
//        Form("%s_core", prefix.c_str()),
//        "DY spline core",
//       *obs_var
//      );

  RooArgList args;
  args.add(*dyCorePdf);
  args.add(*obs_var);

  double mMin = obs_var->getMin();
  double mMax = obs_var->getMax();

  string mhat =
    Form("(2*@1-(%g+%g))/(%g-%g)", mMax, mMin, mMax, mMin);

  string formula = "@0*exp(";

  for(int i = 0; i < order; i++)
  {
    string name = Form("%s_a%d", prefix.c_str(), i);

    RooRealVar* ai =
      new RooRealVar(name.c_str(), name.c_str(),
                     0.0, -0.5, 0.5);

    params.insert({name, ai});
    args.add(*ai);

    if(i > 0) formula += " + ";

    formula += Form("@%d*pow(%s,%d)",
                    args.getSize()-1,
                    mhat.c_str(),
                    i+1);
  }

  formula += ")";

  return new RooGenericPdf(
    prefix.c_str(),
    "DYCore * exp powerlaw correction",
    formula.c_str(),
    args
  );
}

RooAbsPdf* PdfModelBuilder::getDYExpPowerLaw(string prefix, int order, string cat)
{
  // -------------------------------
  // DY core
  // -------------------------------
//  RooAbsPdf* dyCorePdf =
//    new RooDYCore(Form("%s_core", prefix.c_str()),
//                  "DY core",
//                  *obs_var);

  RooAbsPdf* dyCorePdf =
    new RooDYTexpcore(
        Form("%s_core", prefix.c_str()),
        "exp core",
        *obs_var, cat);
                 
//    RooAbsPdf* dyCorePdf =
//      new RooDYSplineCore(
//        Form("%s_core", prefix.c_str()),
//        "DY spline core",
//       *obs_var
//      );

  RooArgList args;
  args.add(*dyCorePdf);
  args.add(*obs_var);

  double mMin = obs_var->getMin();
  double mMax = obs_var->getMax();

  string mhat =
    Form("(2*@1-(%g+%g))/(%g-%g)", mMax, mMin, mMax, mMin);

  string formula = "@0*exp(";

  for(int i = 0; i < order; i++)
  {
    string name = Form("%s_a%d", prefix.c_str(), i);

    RooRealVar* ai =
      new RooRealVar(name.c_str(), name.c_str(),
                     0.0, -0.5, 0.5);

    params.insert({name, ai});
    args.add(*ai);

    if(i > 0) formula += " + ";

    formula += Form("@%d*pow(%s,%d)",
                    args.getSize()-1,
                    mhat.c_str(),
                    i+1);
  }

  formula += ")";

  return new RooGenericPdf(
    prefix.c_str(),
    "DYCore * exp powerlaw correction",
    formula.c_str(),
    args
  );
}

RooAbsPdf* PdfModelBuilder::getDYPowPowerLaw(string prefix, int order, string cat)
{
  // -------------------------------
  // DY core
  // -------------------------------
//  RooAbsPdf* dyCorePdf =
//    new RooDYCore(Form("%s_core", prefix.c_str()),
//                  "DY core",
//                  *obs_var);

  RooAbsPdf* dyCorePdf =
    new RooDYTPowcore(
        Form("%s_core", prefix.c_str()),
        "pow core",
        *obs_var, cat);
                 
//    RooAbsPdf* dyCorePdf =
//      new RooDYSplineCore(
//        Form("%s_core", prefix.c_str()),
//        "DY spline core",
//       *obs_var
//      );

  RooArgList args;
  args.add(*dyCorePdf);
  args.add(*obs_var);

  double mMin = obs_var->getMin();
  double mMax = obs_var->getMax();

  string mhat =
    Form("(2*@1-(%g+%g))/(%g-%g)", mMax, mMin, mMax, mMin);

  string formula = "@0*exp(";

  for(int i = 0; i < order; i++)
  {
    string name = Form("%s_a%d", prefix.c_str(), i);

    RooRealVar* ai =
      new RooRealVar(name.c_str(), name.c_str(),
                     0.0, -1.0, 1.0);

    params.insert({name, ai});
    args.add(*ai);

    if(i > 0) formula += " + ";

    formula += Form("@%d*pow(%s,%d)",
                    args.getSize()-1,
                    mhat.c_str(),
                    i+1);
  }

  formula += ")";

  return new RooGenericPdf(
    prefix.c_str(),
    "DYCore * exp powerlaw correction",
    formula.c_str(),
    args
  );
}

RooAbsPdf* PdfModelBuilder::getDYFewzPowerLaw(string prefix, int order)
{
  // -------------------------------
  // DY core
  // -------------------------------
//  RooAbsPdf* dyCorePdf =
//    new RooDYCore(Form("%s_core", prefix.c_str()),
//                  "DY core",
//                  *obs_var);

//  RooAbsPdf* dyCorePdf =
//    new RooDYBernCore(
//        Form("%s_core", prefix.c_str()),
//        "Bernstein core",
//        *obs_var);
                 
    RooAbsPdf* dyCorePdf =
      new RooDYSplineCore(
        Form("%s_core", prefix.c_str()),
        "DY spline core",
       *obs_var
      );

  RooArgList args;
  args.add(*dyCorePdf);
  args.add(*obs_var);

  double mMin = obs_var->getMin();
  double mMax = obs_var->getMax();

  string mhat =
    Form("(2*@1-(%g+%g))/(%g-%g)", mMax, mMin, mMax, mMin);

  string formula = "@0*exp(";

  for(int i = 0; i < order; i++)
  {
    string name = Form("%s_a%d", prefix.c_str(), i);

    RooRealVar* ai =
      new RooRealVar(name.c_str(), name.c_str(),
                     0.0, -0.5, 0.5);

    params.insert({name, ai});
    args.add(*ai);

    if(i > 0) formula += " + ";

    formula += Form("@%d*pow(%s,%d)",
                    args.getSize()-1,
                    mhat.c_str(),
                    i+1);
  }

  formula += ")";

  return new RooGenericPdf(
    prefix.c_str(),
    "DYCore * exp powerlaw correction",
    formula.c_str(),
    args
  );
}

/*
RooAbsPdf* PdfModelBuilder::getDYLaurent(string prefix, int order)
{
  // --- DY core (already a normalized RooAbsPdf) ---
  RooAbsPdf* dyCorePdf =
    new RooDYCore(Form("%s_core", prefix.c_str()),
                  "DY core",
                  *obs_var);
 
//    RooAbsPdf* dyCorePdf =
//      new RooDYSplineCore(
//        Form("%s_core", prefix.c_str()),
//        "DY spline core",
//       *obs_var
//      );

  RooArgList args;
  args.add(*dyCorePdf); // @0
  args.add(*obs_var);   // @1 = m

  // --- mass range ---
  double mMin = obs_var->getMin();
  double mMax = obs_var->getMax();

  // expansion center
  double mCenter = 0.5 * (mMin + mMax);

  // centered inverse variable
  string y =
    Form("(1./@1 - 1./%g)", mCenter);

  // --- build formula ---
  string formula = "@0*exp(";

  for(int i = 0; i < order; i++)
  {
    string name = Form("%s_a%d", prefix.c_str(), i);

    // tighter range helps a lot
    RooRealVar* ai =
      new RooRealVar(name.c_str(),
                     name.c_str(),
                     0.0,
                     -0.1,
                      0.1);

    params.insert({name, ai});
    args.add(*ai);

    if(i > 0) formula += " + ";

    formula += Form("@%d*pow(%s,%d)",
                    args.getSize()-1,
                    y.c_str(),
                    i+1);
  }

  formula += ")";

  return new RooGenericPdf(
    prefix.c_str(),
    "DYCore * centered Laurent correction",
    formula.c_str(),
    args
  );
}
*/

RooAbsPdf* PdfModelBuilder::getDYLaurent(string prefix, int order)
{
  // -------------------------------
  // DY core
  // -------------------------------
  RooAbsPdf* dyCorePdf =
    new RooDYCore(Form("%s_core", prefix.c_str()),
                  "DY core",
                  *obs_var);

//  RooAbsPdf* dyCorePdf =
//    new RooDYBernCore(
//        Form("%s_core", prefix.c_str()),
//        "Bernstein core",
//        *obs_var);
                 
//    RooAbsPdf* dyCorePdf =
//      new RooDYSplineCore(
//        Form("%s_core", prefix.c_str()),
//        "DY spline core",
//       *obs_var
//      );

  RooArgList args;
  args.add(*dyCorePdf);   // @0
  args.add(*obs_var);     // @1

  // -------------------------------
  // normalized inverse-mass variable
  // -------------------------------
  double mMin = obs_var->getMin();
  double mMax = obs_var->getMax();
  double mCenter = 0.5*(mMin+mMax);

  double ymax = std::max(
      fabs(1./mMin - 1./mCenter),
      fabs(1./mMax - 1./mCenter));

  string y =
      Form("((1./@1-1./%g)/%g)",
           mCenter,
           ymax);

  constexpr double scale = 0.50;

  string formula = Form("@0*(1+%.3f*(", scale);

  for(int i=0;i<order;i++)
  {
      string name = Form("%s_a%d", prefix.c_str(), i);

      RooRealVar* ai =
          new RooRealVar(name.c_str(),
                         name.c_str(),
                         0.0,
                         -0.70,
                          0.70);

      params.insert({name, ai});
      args.add(*ai);

      if(i>0)
          formula += " + ";

      formula += Form("@%d*pow(%s,%d)",
                      args.getSize()-1,
                      y.c_str(),
                      i+1);
  }

  formula += "))";

  return new RooGenericPdf(
      prefix.c_str(),
      "DYCore * Laurent correction",
      formula.c_str(),
      args);
}

RooAbsPdf* PdfModelBuilder::getDYBernLaurent(string prefix, int order)
{
  // -------------------------------
  // DY core
  // -------------------------------
//  RooAbsPdf* dyCorePdf =
//    new RooDYCore(Form("%s_core", prefix.c_str()),
//                  "DY core",
//                  *obs_var);

  RooAbsPdf* dyCorePdf =
    new RooDYBernCore(
        Form("%s_core", prefix.c_str()),
        "Bernstein core",
        *obs_var);
                 
//    RooAbsPdf* dyCorePdf =
//      new RooDYSplineCore(
//        Form("%s_core", prefix.c_str()),
//        "DY spline core",
//       *obs_var
//      );

  RooArgList args;
  args.add(*dyCorePdf);   // @0
  args.add(*obs_var);     // @1

  // -------------------------------
  // normalized inverse-mass variable
  // -------------------------------
  double mMin = obs_var->getMin();
  double mMax = obs_var->getMax();
  double mCenter = 0.5*(mMin+mMax);

  double ymax = std::max(
      fabs(1./mMin - 1./mCenter),
      fabs(1./mMax - 1./mCenter));

  string y =
      Form("((1./@1-1./%g)/%g)",
           mCenter,
           ymax);

  constexpr double scale = 0.50;

  string formula = Form("@0*(1+%.3f*(", scale);

  for(int i=0;i<order;i++)
  {
      string name = Form("%s_a%d", prefix.c_str(), i);

      RooRealVar* ai =
          new RooRealVar(name.c_str(),
                         name.c_str(),
                         0.0,
                         -0.70,
                          0.70);

      params.insert({name, ai});
      args.add(*ai);

      if(i>0)
          formula += " + ";

      formula += Form("@%d*pow(%s,%d)",
                      args.getSize()-1,
                      y.c_str(),
                      i+1);
  }

  formula += "))";

  return new RooGenericPdf(
      prefix.c_str(),
      "DYCore * Laurent correction",
      formula.c_str(),
      args);
}

RooAbsPdf* PdfModelBuilder::getDYExpLaurent(string prefix, int order, string cat)
{
  // -------------------------------
  // DY core
  // -------------------------------
//  RooAbsPdf* dyCorePdf =
//    new RooDYCore(Form("%s_core", prefix.c_str()),
//                  "DY core",
//                  *obs_var);

  RooAbsPdf* dyCorePdf =
    new RooDYTexpcore(
        Form("%s_core", prefix.c_str()),
        "exp core",
        *obs_var, cat);
                 
//    RooAbsPdf* dyCorePdf =
//      new RooDYSplineCore(
//        Form("%s_core", prefix.c_str()),
//        "DY spline core",
//       *obs_var
//      );

  RooArgList args;
  args.add(*dyCorePdf);   // @0
  args.add(*obs_var);     // @1

  // -------------------------------
  // normalized inverse-mass variable
  // -------------------------------
  double mMin = obs_var->getMin();
  double mMax = obs_var->getMax();
  double mCenter = 0.5*(mMin+mMax);

  double ymax = std::max(
      fabs(1./mMin - 1./mCenter),
      fabs(1./mMax - 1./mCenter));

  string y =
      Form("((1./@1-1./%g)/%g)",
           mCenter,
           ymax);

  constexpr double scale = 0.50;

  string formula = Form("@0*(1+%.3f*(", scale);

  for(int i=0;i<order;i++)
  {
      string name = Form("%s_a%d", prefix.c_str(), i);

      RooRealVar* ai =
          new RooRealVar(name.c_str(),
                         name.c_str(),
                         0.0,
                         -0.70,
                          0.70);

      params.insert({name, ai});
      args.add(*ai);

      if(i>0)
          formula += " + ";

      formula += Form("@%d*pow(%s,%d)",
                      args.getSize()-1,
                      y.c_str(),
                      i+1);
  }

  formula += "))";

  return new RooGenericPdf(
      prefix.c_str(),
      "DYCore * Laurent correction",
      formula.c_str(),
      args);
}

RooAbsPdf* PdfModelBuilder::getDYPowLaurent(string prefix, int order, string cat)
{
  // -------------------------------
  // DY core
  // -------------------------------
//  RooAbsPdf* dyCorePdf =
//    new RooDYCore(Form("%s_core", prefix.c_str()),
//                  "DY core",
//                  *obs_var);

  RooAbsPdf* dyCorePdf =
    new RooDYTPowcore(
        Form("%s_core", prefix.c_str()),
        "pow core",
        *obs_var, cat);
                 
//    RooAbsPdf* dyCorePdf =
//      new RooDYSplineCore(
//        Form("%s_core", prefix.c_str()),
//        "DY spline core",
//       *obs_var
//      );

  RooArgList args;
  args.add(*dyCorePdf);   // @0
  args.add(*obs_var);     // @1

  // -------------------------------
  // normalized inverse-mass variable
  // -------------------------------
  double mMin = obs_var->getMin();
  double mMax = obs_var->getMax();
  double mCenter = 0.5*(mMin+mMax);

  double ymax = std::max(
      fabs(1./mMin - 1./mCenter),
      fabs(1./mMax - 1./mCenter));

  string y =
      Form("((1./@1-1./%g)/%g)",
           mCenter,
           ymax);

  constexpr double scale = 0.50;

  string formula = Form("@0*(1+%.3f*(", scale);

  for(int i=0;i<order;i++)
  {
      string name = Form("%s_a%d", prefix.c_str(), i);

      RooRealVar* ai =
          new RooRealVar(name.c_str(),
                         name.c_str(),
                         0.0,
                         -1.0,
                          1.0);

      params.insert({name, ai});
      args.add(*ai);

      if(i>0)
          formula += " + ";

      formula += Form("@%d*pow(%s,%d)",
                      args.getSize()-1,
                      y.c_str(),
                      i+1);
  }

  formula += "))";

  return new RooGenericPdf(
      prefix.c_str(),
      "DYCore * Laurent correction",
      formula.c_str(),
      args);
}

RooAbsPdf* PdfModelBuilder::getDYFewzLaurent(string prefix, int order)
{
  // -------------------------------
  // DY core
  // -------------------------------
//  RooAbsPdf* dyCorePdf =
//    new RooDYCore(Form("%s_core", prefix.c_str()),
//                  "DY core",
//                  *obs_var);

//  RooAbsPdf* dyCorePdf =
//    new RooDYBernCore(
//        Form("%s_core", prefix.c_str()),
//        "Bernstein core",
//        *obs_var);
                 
    RooAbsPdf* dyCorePdf =
      new RooDYSplineCore(
        Form("%s_core", prefix.c_str()),
        "DY spline core",
       *obs_var
      );

  RooArgList args;
  args.add(*dyCorePdf);   // @0
  args.add(*obs_var);     // @1

  // -------------------------------
  // normalized inverse-mass variable
  // -------------------------------
  double mMin = obs_var->getMin();
  double mMax = obs_var->getMax();
  double mCenter = 0.5*(mMin+mMax);

  double ymax = std::max(
      fabs(1./mMin - 1./mCenter),
      fabs(1./mMax - 1./mCenter));

  string y =
      Form("((1./@1-1./%g)/%g)",
           mCenter,
           ymax);

  constexpr double scale = 0.50;

  string formula = Form("@0*(1+%.3f*(", scale);

  for(int i=0;i<order;i++)
  {
      string name = Form("%s_a%d", prefix.c_str(), i);

      RooRealVar* ai =
          new RooRealVar(name.c_str(),
                         name.c_str(),
                         0.0,
                         -0.70,
                          0.70);

      params.insert({name, ai});
      args.add(*ai);

      if(i>0)
          formula += " + ";

      formula += Form("@%d*pow(%s,%d)",
                      args.getSize()-1,
                      y.c_str(),
                      i+1);
  }

  formula += "))";

  return new RooGenericPdf(
      prefix.c_str(),
      "DYCore * Laurent correction",
      formula.c_str(),
      args);
}

// ============================================================
// MC corrected DY cores
//
// These use RooDYCoreMod / RooDYSplineCoreMod: the fixed DY shape
// multiplied by a per-category third order Chebyshev correction
// whose coefficients were fitted on the MC (see
// Background/test/fitDYshapes.cpp and the CSVs in
// Background/DYshapes).
//
// On top of that corrected core the usual floating empirical
// correction (Chebyshev / power law / Laurent) is applied, in
// exactly the same way as for the getDY* / getDYFewz* families.
// ============================================================

RooAbsPdf* PdfModelBuilder::getDYModChebychev(string prefix, int empOrder, string cat)
{
  // -------------------------------
  // DY core (MC corrected)
  // -------------------------------
  RooAbsPdf* dyCorePdf =
    new RooDYCoreMod(
        Form("%s_core", prefix.c_str()),
        "DY core (MC corrected)",
        *obs_var, cat);

  RooArgList args;
  args.add(*dyCorePdf);   // @0
  args.add(*obs_var);     // @1

  // map m -> [-1,1]
  const double mMin = obs_var->getMin();
  const double mMax = obs_var->getMax();

  string mhat =
      Form("(2*@1-(%g+%g))/(%g-%g)",
           mMax, mMin,
           mMax, mMin);

  constexpr double scale = 0.50;

  string formula = Form("@0*(1+%.3f*(", scale);

  for(int i=1;i<=empOrder;i++)
  {
      string name = Form("%s_a%d", prefix.c_str(), i);

      RooRealVar* ai =
          new RooRealVar(name.c_str(),
                         name.c_str(),
                         0.0,
                         -1.0,
                          1.0);

      params.insert({name, ai});
      args.add(*ai);

      if(i>1)
          formula += " + ";

      formula += Form("@%d*cos(%d*acos(%s))",
                      args.getSize()-1,
                      i,
                      mhat.c_str());
  }

  formula += "))";

  return new RooGenericPdf(
      prefix.c_str(),
      "DYCoreMod * Chebyshev correction",
      formula.c_str(),
      args);
}

RooAbsPdf* PdfModelBuilder::getDYModPowerLaw(string prefix, int order, string cat)
{
  // -------------------------------
  // DY core (MC corrected)
  // -------------------------------
  RooAbsPdf* dyCorePdf =
    new RooDYCoreMod(
        Form("%s_core", prefix.c_str()),
        "DY core (MC corrected)",
        *obs_var, cat);

  RooArgList args;
  args.add(*dyCorePdf);
  args.add(*obs_var);

  double mMin = obs_var->getMin();
  double mMax = obs_var->getMax();

  string mhat =
    Form("(2*@1-(%g+%g))/(%g-%g)", mMax, mMin, mMax, mMin);

  string formula = "@0*exp(";

  for(int i = 0; i < order; i++)
  {
    string name = Form("%s_a%d", prefix.c_str(), i);

    RooRealVar* ai =
      new RooRealVar(name.c_str(), name.c_str(),
                     0.0, -0.5, 0.5);

    params.insert({name, ai});
    args.add(*ai);

    if(i > 0) formula += " + ";

    formula += Form("@%d*pow(%s,%d)",
                    args.getSize()-1,
                    mhat.c_str(),
                    i+1);
  }

  formula += ")";

  return new RooGenericPdf(
    prefix.c_str(),
    "DYCoreMod * exp powerlaw correction",
    formula.c_str(),
    args
  );
}

RooAbsPdf* PdfModelBuilder::getDYModLaurent(string prefix, int order, string cat)
{
  // -------------------------------
  // DY core (MC corrected)
  // -------------------------------
  RooAbsPdf* dyCorePdf =
    new RooDYCoreMod(
        Form("%s_core", prefix.c_str()),
        "DY core (MC corrected)",
        *obs_var, cat);

  RooArgList args;
  args.add(*dyCorePdf);   // @0
  args.add(*obs_var);     // @1

  // -------------------------------
  // normalized inverse-mass variable
  // -------------------------------
  double mMin = obs_var->getMin();
  double mMax = obs_var->getMax();
  double mCenter = 0.5*(mMin+mMax);

  double ymax = std::max(
      fabs(1./mMin - 1./mCenter),
      fabs(1./mMax - 1./mCenter));

  string y =
      Form("((1./@1-1./%g)/%g)",
           mCenter,
           ymax);

  constexpr double scale = 0.50;

  string formula = Form("@0*(1+%.3f*(", scale);

  for(int i=0;i<order;i++)
  {
      string name = Form("%s_a%d", prefix.c_str(), i);

      RooRealVar* ai =
          new RooRealVar(name.c_str(),
                         name.c_str(),
                         0.0,
                         -0.70,
                          0.70);

      params.insert({name, ai});
      args.add(*ai);

      if(i>0)
          formula += " + ";

      formula += Form("@%d*pow(%s,%d)",
                      args.getSize()-1,
                      y.c_str(),
                      i+1);
  }

  formula += "))";

  return new RooGenericPdf(
      prefix.c_str(),
      "DYCoreMod * Laurent correction",
      formula.c_str(),
      args);
}

RooAbsPdf* PdfModelBuilder::getDYFewzModChebychev(string prefix, int empOrder, string cat)
{
  // -------------------------------
  // DY spline core (MC corrected)
  // -------------------------------
  RooAbsPdf* dyCorePdf =
    new RooDYSplineCoreMod(
        Form("%s_core", prefix.c_str()),
        "DY spline core (MC corrected)",
        *obs_var, cat);

  RooArgList args;
  args.add(*dyCorePdf);   // @0
  args.add(*obs_var);     // @1

  // map m -> [-1,1]
  const double mMin = obs_var->getMin();
  const double mMax = obs_var->getMax();

  string mhat =
      Form("(2*@1-(%g+%g))/(%g-%g)",
           mMax, mMin,
           mMax, mMin);

  constexpr double scale = 0.50;

  string formula = Form("@0*(1+%.3f*(", scale);

  for(int i=1;i<=empOrder;i++)
  {
      string name = Form("%s_a%d", prefix.c_str(), i);

      RooRealVar* ai =
          new RooRealVar(name.c_str(),
                         name.c_str(),
                         0.0,
                         -1.0,
                          1.0);

      params.insert({name, ai});
      args.add(*ai);

      if(i>1)
          formula += " + ";

      formula += Form("@%d*cos(%d*acos(%s))",
                      args.getSize()-1,
                      i,
                      mhat.c_str());
  }

  formula += "))";

  return new RooGenericPdf(
      prefix.c_str(),
      "DYSplineCoreMod * Chebyshev correction",
      formula.c_str(),
      args);
}

RooAbsPdf* PdfModelBuilder::getDYFewzModPowerLaw(string prefix, int order, string cat)
{
  // -------------------------------
  // DY spline core (MC corrected)
  // -------------------------------
  RooAbsPdf* dyCorePdf =
    new RooDYSplineCoreMod(
        Form("%s_core", prefix.c_str()),
        "DY spline core (MC corrected)",
        *obs_var, cat);

  RooArgList args;
  args.add(*dyCorePdf);
  args.add(*obs_var);

  double mMin = obs_var->getMin();
  double mMax = obs_var->getMax();

  string mhat =
    Form("(2*@1-(%g+%g))/(%g-%g)", mMax, mMin, mMax, mMin);

  string formula = "@0*exp(";

  for(int i = 0; i < order; i++)
  {
    string name = Form("%s_a%d", prefix.c_str(), i);

    RooRealVar* ai =
      new RooRealVar(name.c_str(), name.c_str(),
                     0.0, -0.5, 0.5);

    params.insert({name, ai});
    args.add(*ai);

    if(i > 0) formula += " + ";

    formula += Form("@%d*pow(%s,%d)",
                    args.getSize()-1,
                    mhat.c_str(),
                    i+1);
  }

  formula += ")";

  return new RooGenericPdf(
    prefix.c_str(),
    "DYSplineCoreMod * exp powerlaw correction",
    formula.c_str(),
    args
  );
}

RooAbsPdf* PdfModelBuilder::getDYFewzModLaurent(string prefix, int order, string cat)
{
  // -------------------------------
  // DY spline core (MC corrected)
  // -------------------------------
  RooAbsPdf* dyCorePdf =
    new RooDYSplineCoreMod(
        Form("%s_core", prefix.c_str()),
        "DY spline core (MC corrected)",
        *obs_var, cat);

  RooArgList args;
  args.add(*dyCorePdf);   // @0
  args.add(*obs_var);     // @1

  // -------------------------------
  // normalized inverse-mass variable
  // -------------------------------
  double mMin = obs_var->getMin();
  double mMax = obs_var->getMax();
  double mCenter = 0.5*(mMin+mMax);

  double ymax = std::max(
      fabs(1./mMin - 1./mCenter),
      fabs(1./mMax - 1./mCenter));

  string y =
      Form("((1./@1-1./%g)/%g)",
           mCenter,
           ymax);

  constexpr double scale = 0.50;

  string formula = Form("@0*(1+%.3f*(", scale);

  for(int i=0;i<order;i++)
  {
      string name = Form("%s_a%d", prefix.c_str(), i);

      RooRealVar* ai =
          new RooRealVar(name.c_str(),
                         name.c_str(),
                         0.0,
                         -0.70,
                          0.70);

      params.insert({name, ai});
      args.add(*ai);

      if(i>0)
          formula += " + ";

      formula += Form("@%d*pow(%s,%d)",
                      args.getSize()-1,
                      y.c_str(),
                      i+1);
  }

  formula += "))";

  return new RooGenericPdf(
      prefix.c_str(),
      "DYSplineCoreMod * Laurent correction",
      formula.c_str(),
      args);
}


RooAbsPdf* PdfModelBuilder::getBWZExpPoly(string prefix, int order)
{
  RooArgList args;

  // observable
  args.add(*obs_var); // @0

  // ---------------------------------
  // fixed Z parameters
  // ---------------------------------

  RooRealVar* mZ =
    new RooRealVar(
      Form("%s_mZ",prefix.c_str()),
      "Z mass",
      91.1876
    );
  mZ->setConstant(true);

  RooRealVar* gammaZ =
    new RooRealVar(
      Form("%s_gammaZ",prefix.c_str()),
      "Z width",
      2.4952
    );
  gammaZ->setConstant(true);

  args.add(*mZ);      // @1
  args.add(*gammaZ);  // @2

  // ---------------------------------
  // scaled variable
  //
  // x=(m-125)/25
  // ---------------------------------

  std::string x = "((@0-125.0)/25.0)";

  std::string formula;

  // ---------------------------------
  // order = 0:
  // pure BWZ, no exponential term
  // ---------------------------------

  if(order == 0)
  {
    formula =
      "(@2/"
      "((@0-@1)*(@0-@1)+pow(@2/2.,2)))";
  }

  // ---------------------------------
  // order >= 1:
  // BWZ * exponential polynomial
  // ---------------------------------

  else
  {
    formula = "(@2*exp(";

    // ---------------------------------
    // exponential polynomial
    // ---------------------------------

    for(int i=0; i<order; i++)
    {
      std::string name =
        Form("%s_p%d",prefix.c_str(),i);

      RooRealVar* p =
        new RooRealVar(
          name.c_str(),
          name.c_str(),
          0.0,
          -5.0,
           5.0
        );

      params.insert({name,p});

      args.add(*p);

      int idx = args.getSize()-1;

      if(i>0)
        formula += " + ";

      if(i==0)
      {
        formula += Form(
          "@%d*(%s)",
          idx,
          x.c_str()
        );
      }
      else
      {
        formula += Form(
          "@%d*pow(%s,%d)",
          idx,
          x.c_str(),
          i+1
        );
      }
    }

    formula += ")";

    // ---------------------------------
    // BWZ denominator
    // ---------------------------------

    formula +=
      "/((@0-@1)*(@0-@1)"
      "+pow(@2/2.,2)))";
  }

  std::cout
    << "BWZExpPoly formula = "
    << formula
    << std::endl;

  return new RooGenericPdf(
    prefix.c_str(),
    prefix.c_str(),
    formula.c_str(),
    args
  );
}

RooAbsPdf* PdfModelBuilder::getKeysPdf(string prefix){
  if (!keysPdfAttributesSet){
    cerr << "ERROR -- keysPdf attributes not set" << endl;
    exit(1);
  }
  return new RooKeysPdf(prefix.c_str(),prefix.c_str(),*obs_var,*keysPdfData,RooKeysPdf::MirrorBoth,keysPdfRho);
}

RooAbsPdf* PdfModelBuilder::getPdfFromFile(string &prefix){
  vector<string> details;
  split(details,prefix,boost::is_any_of(","));

  string fname = details[2];
  string wsname = details[1];
  string pdfname = details[0];

  TFile *tempFile = TFile::Open(fname.c_str());
  if (!tempFile){
    cerr << "PdfModelBuilder::getPdfFromFile -- file not found " << fname << endl;
    assert(0);
  }
  RooWorkspace *tempWS = (RooWorkspace*)tempFile->Get(wsname.c_str());
  if (!tempWS){
    cerr << "PdfModelBuilder::getPdfFromFile -- workspace not found " << wsname << endl;
    assert(0);
  }
  RooAbsPdf *tempPdf = (RooAbsPdf*)tempWS->pdf(pdfname.c_str());
  if (!tempPdf){
    cerr << "PdfModelBuilder::getPdfFromFile -- pdf not found " << pdfname << endl;
    assert(0);
  }
  prefix = pdfname;
  RooAbsPdf *pdf = (RooAbsPdf*)tempPdf->Clone(prefix.c_str());
  tempFile->Close();
  delete tempFile;
  return pdf;
}

RooAbsPdf* PdfModelBuilder::getExponentialSingle(string prefix, int order){
  
  if (order%2==0){
    cerr << "ERROR -- addExponential -- only odd number of params allowed" << endl;
    return NULL;
  }
  else {
    int nfracs=(order-1)/2;
    int nexps=order-nfracs;
    assert(nfracs==nexps-1);
    RooArgList *fracs = new RooArgList();
    RooArgList *exps = new RooArgList();
    for (int i=1; i<=nfracs; i++){
      string name =  Form("%s_f%d",prefix.c_str(),i);
      params.insert(pair<string,RooRealVar*>(name, new RooRealVar(name.c_str(),name.c_str(),0.9-float(i-1)*1./nfracs,0.0001,0.9999)));
      fracs->add(*params[name]);
    }
    for (int i=1; i<=nexps; i++){
      string name =  Form("%s_p%d",prefix.c_str(),i);
      string ename =  Form("%s_e%d",prefix.c_str(),i);
      params.insert(pair<string,RooRealVar*>(name, new RooRealVar(name.c_str(),name.c_str(),TMath::Max(-1.,-0.04*(i+1)),-1.,0.)));
      utilities.insert(pair<string,RooAbsPdf*>(ename, new RooExponential(ename.c_str(),ename.c_str(),*obs_var,*params[name])));
      exps->add(*utilities[ename]);
    }
    //fracs->Print("v");
    //exps->Print("v");
    RooAbsPdf *exp = new RooAddPdf(prefix.c_str(),prefix.c_str(),*exps,*fracs,true);
    //exp->Print("v");
    cout << "--------------------------" << endl;
    return exp;
    //bkgPdfs.insert(pair<string,RooAbsPdf*>(exp->GetName(),exp));

  }
}


void PdfModelBuilder::addBkgPdf(string type, int nParams, string name, bool cache){
 
  if (!obs_var_set){
    cerr << "ERROR -- obs Var has not been set!" << endl;
    exit(1);
  }
  bool found=false;
  for (vector<string>::iterator it=recognisedPdfTypes.begin(); it!=recognisedPdfTypes.end(); it++){
    if (*it==type) found=true;
  }
  if (!found){
    cerr << "Pdf of type " << type << " is not recognised!" << endl;
    exit(1);
  }
  RooAbsPdf *pdf=0;// avoid uninitialised variable error in cmssw  

//  if (type=="Bernstein") pdf = getBernstein(name,nParams);
//  if (type=="Exponential") pdf = getExponentialSingle(name,nParams);
  if (type == "ModBreitWigner") {
    pdf = getModBreitWigner(name);
  }
//  if (type=="PowerLaw") pdf = getPowerLawSingle(name,nParams);
  if (type=="Laurent") pdf = getLaurentSeries(name,nParams);
  if (type=="Bernstein") pdf = getBernstein(name,nParams);
  if (type=="Exponential") pdf = getExponentialSingle(name,nParams);
  if (type=="PowerLaw") pdf = getPowerLawSingle(name,nParams);
  if (type=="DYTheory") pdf = getDYTheory(name,nParams);
  if (type=="DYTheory_cheb") pdf = getDYTheory(name,nParams);
  if (type=="DYTheory_pow") pdf = getDYPowerLaw(name,nParams);
  if (type=="DYTheory_lau") pdf = getDYLaurent(name,nParams);
  if (type=="DYTheory_bern") pdf = getDYBernstein(name,nParams);
  if (type=="BWZ_exp") pdf = getBWZExpPoly(name,nParams);
  if (type=="BWZ_gamma") pdf = getBWZGamma(name);
  if (type=="2PowerLaw") pdf = get2PowerLaw(name);
  if (type=="2Exp") pdf = get2Exp(name);
  if (type=="KeysPdf") pdf = getKeysPdf(name);
  if (type=="File") pdf = getPdfFromFile(name);

  if (cache) {
    wsCache->import(*pdf);
    RooAbsPdf *cachePdf = wsCache->pdf(pdf->GetName());
    bkgPdfs.insert(pair<string,RooAbsPdf*>(cachePdf->GetName(),cachePdf));
  }
  else {
    bkgPdfs.insert(pair<string,RooAbsPdf*>(pdf->GetName(),pdf));
  }

}

void PdfModelBuilder::setKeysPdfAttributes(RooDataSet *data, double rho){
  keysPdfData = data;
  keysPdfRho = rho;
  keysPdfAttributesSet=true;
}

void PdfModelBuilder::setSignalPdf(RooAbsPdf *pdf, RooRealVar *norm){
  sigPdf=pdf;
  sigNorm=norm;
  signal_set=true;
}

void PdfModelBuilder::setSignalPdfFromMC(RooDataSet *data){
  
  RooDataHist *sigMCBinned = new RooDataHist(Form("roohist_%s",data->GetName()),Form("roohist_%s",data->GetName()),RooArgSet(*obs_var),*data);
  sigPdf = new RooHistPdf(Form("pdf_%s",data->GetName()),Form("pdf_%s",data->GetName()),RooArgSet(*obs_var),*sigMCBinned);
  sigNorm = new RooConstVar(Form("sig_events_%s",data->GetName()),Form("sig_events_%s",data->GetName()),data->sumEntries());
  signal_set=true;
}

void PdfModelBuilder::makeSBPdfs(bool cache){
  
  if (!signal_set){
    cerr << "ERROR - no signal model set!" << endl;
    exit(1);
  }
  if (!signal_modifier_set){
    cerr << "ERROR - no signal modifier set!" << endl;
    exit(1);
  }
 
  if (sigNorm) {
    sigYield = new RooProduct("sig_yield","sig_yield",RooArgSet(*signalModifier,*sigNorm));
  }
  else {
    sigYield = signalModifier;
  }
  bkgYield = new RooRealVar("bkg_yield","bkg_yield",1000.,0.,1.e6);

  for (map<string,RooAbsPdf*>::iterator bkg=bkgPdfs.begin(); bkg!=bkgPdfs.end(); bkg++){
    RooAbsPdf *sbMod = new RooAddPdf(Form("sb_%s",bkg->first.c_str()),Form("sb_%s",bkg->first.c_str()),RooArgList(*(bkg->second),*sigPdf),RooArgList(*bkgYield,*sigYield));
    if (cache) {
      wsCache->import(*sbMod,RecycleConflictNodes());
      RooAbsPdf *cachePdf = (RooAbsPdf*)wsCache->pdf(sbMod->GetName());
      signalModifier = (RooRealVar*)wsCache->var(signalModifier->GetName());
      sbPdfs.insert(pair<string,RooAbsPdf*>(cachePdf->GetName(),cachePdf));
    }
    else {
      sbPdfs.insert(pair<string,RooAbsPdf*>(sbMod->GetName(),sbMod));
    }
  }
}

map<string,RooAbsPdf*> PdfModelBuilder::getBkgPdfs(){
  return bkgPdfs;
}

map<string,RooAbsPdf*> PdfModelBuilder::getSBPdfs(){
  return sbPdfs;
}

RooAbsPdf* PdfModelBuilder::getSigPdf(){
  return sigPdf;
}

void PdfModelBuilder::plotPdfsToData(RooAbsData *data, int binning, string name, bool bkgOnly,string specificPdfName){
  
  TCanvas *canv = new TCanvas();
  bool specPdf=false;
  if (specificPdfName!="") specPdf=true;

  map<string,RooAbsPdf*> pdfSet;
  if (bkgOnly) pdfSet = bkgPdfs;
  else pdfSet = sbPdfs;
  
  for (map<string,RooAbsPdf*>::iterator it=pdfSet.begin(); it!=pdfSet.end(); it++){
    if (specPdf && it->first!=specificPdfName && specificPdfName!="NONE") continue;
    RooPlot *plot = obs_var->frame();
    data->plotOn(plot,Binning(binning));
    if (specificPdfName!="NONE") {
	 it->second->plotOn(plot);
	 it->second->paramOn(plot,RooFit::Layout(0.34,0.96,0.89),RooFit::Format("NEA",AutoPrecision(1)));
    }	
    plot->Draw();
    canv->Print(Form("%s_%s.pdf",name.c_str(),it->first.c_str()));
    canv->Print(Form("%s_%s.png",name.c_str(),it->first.c_str()));
  }
  delete canv;
}

void PdfModelBuilder::fitToData(RooAbsData *data, bool bkgOnly, bool cache, bool print){
  
  map<string,RooAbsPdf*> pdfSet;
  if (bkgOnly) pdfSet = bkgPdfs;
  else pdfSet = sbPdfs;

  for (map<string,RooAbsPdf*>::iterator it=pdfSet.begin(); it!=pdfSet.end(); it++){
    RooFitResult *fit = (RooFitResult*)it->second->fitTo(*data,Save(true));
    if (print){
      cout << "Fit Res Before: " << endl;
      fit->floatParsInit().Print("v");
      cout << "Fit Res After: " << endl;
      fit->floatParsFinal().Print("v");
    }
    if (cache) {
      RooArgSet *fitargs = (RooArgSet*)it->second->getParameters(*obs_var);
      // remove the signal strength since this will be set AFTER fitting the background 
      fitargs->remove(*signalModifier); 
      wsCache->defineSet(Form("%s_params",it->first.c_str()),*fitargs);
      wsCache->defineSet(Form("%s_observs",it->first.c_str()),*obs_var);
      wsCache->saveSnapshot(it->first.c_str(),*fitargs,true);
      if (print) {
        cout << "Cached values: " << endl;
        fitargs->Print("v");
      }
    }
  }
  if (bkgOnly) bkgHasFit=true;
  else sbHasFit=true;
}

void PdfModelBuilder::setSeed(int seed){
  RooRandom::randomGenerator()->SetSeed(seed);
}

RooDataSet* PdfModelBuilder::makeHybridDataset(vector<float> switchOverMasses, vector<RooDataSet*> dataForHybrid){
  
  assert(switchOverMasses.size()==dataForHybrid.size()-1);

  vector<string> cut_strings;
  cut_strings.push_back("cutstring0");
  obs_var->setRange("cutstring0",obs_var->getMin(),switchOverMasses[0]);
  for (unsigned int i=1; i<switchOverMasses.size(); i++){
    cut_strings.push_back(Form("cutstring%d",i));
    obs_var->setRange(Form("cutstring%d",i),switchOverMasses[i-1],switchOverMasses[i]);
  }
  cut_strings.push_back(Form("cutstring%d",int(switchOverMasses.size())));
  obs_var->setRange(Form("cutstring%d",int(switchOverMasses.size())),switchOverMasses[switchOverMasses.size()-1],obs_var->getMax());
  
  obs_var->Print("v");
  assert(cut_strings.size()==dataForHybrid.size());
  
	RooDataSet *data=0;// avoid uninitialised variable error in cmssw  
  for (unsigned int i=0; i<dataForHybrid.size(); i++){
    RooDataSet *cutData = (RooDataSet*)dataForHybrid[i]->reduce(Name("hybridToy"),Title("hybridToy"),CutRange(cut_strings[i].c_str()));
    //RooDataSet *cutData = new RooDataSet("hybridToy","hybridToy",RooArgSet(*obs_var),Import(*dataForHybrid[i]),CutRange(cut_strings[i].c_str()));
    if (i==0) data=cutData;
    else data->append(*cutData);
  }
  return data;
}

void PdfModelBuilder::throwHybridToy(string postfix, int nEvents, vector<float> switchOverMasses, vector<string> functions, bool bkgOnly, bool binned, bool poisson, bool cache){
  
  assert(switchOverMasses.size()==functions.size()-1);
  toyHybridData.clear();

  // have to throw unbinned for the hybrid
  throwToy(postfix,nEvents,bkgOnly,false,poisson,cache);

  vector<RooDataSet*> dataForHybrid;
  string hybridName = "hybrid";
  for (vector<string>::iterator func=functions.begin(); func!=functions.end(); func++){
    hybridName += "_"+*func;
    for (map<string,RooDataSet*>::iterator it=toyDataSet.begin(); it!=toyDataSet.end(); it++){
      if (it->first.find(*func)!=string::npos){
        dataForHybrid.push_back(it->second);
      }
    }
  }
  if (dataForHybrid.size()!=functions.size()){
    cerr << "One of the requested hybrid functions has not been found" << endl;
    exit(1);
  }

  RooDataSet *hybridData = makeHybridDataset(switchOverMasses,dataForHybrid);
  toyHybridData.clear();
  if (binned) {
    RooDataHist *hybridDataHist = hybridData->binnedClone();
    hybridDataHist->SetName(Form("%s_%s",hybridName.c_str(),postfix.c_str()));
    toyHybridData.insert(pair<string,RooAbsData*>(hybridDataHist->GetName(),hybridDataHist));
  }
  else {
    hybridData->SetName(Form("%s_%s",hybridName.c_str(),postfix.c_str()));
    toyHybridData.insert(pair<string,RooAbsData*>(hybridData->GetName(),hybridData));
  }
}

void PdfModelBuilder::throwToy(string postfix, int nEvents, bool bkgOnly, bool binned, bool poisson, bool cache){

  toyData.clear();
  toyDataSet.clear();
  toyDataHist.clear();
  map<string,RooAbsPdf*> pdfSet;
  if (bkgOnly) {
    pdfSet = bkgPdfs;
    if (!bkgHasFit) cerr << "WARNING -- bkg has not been fit to data. Are you sure this is wise?" << endl; 
  }
  else {
    pdfSet = sbPdfs;
    if (!sbHasFit) cerr << "WARNING -- sb has not been fit to data. Are you sure this is wise?" << endl;
  }
  
  for (map<string,RooAbsPdf*>::iterator it=pdfSet.begin(); it!=pdfSet.end(); it++){
    if (cache) {
      wsCache->loadSnapshot(it->first.c_str());
      cout << "Loaded snapshot, params at.." << endl;
      it->second->getVariables()->Print("v");
    }
    RooAbsData *toy;
    if (binned){
      RooDataHist *toyHist;
      if (poisson) toyHist = it->second->generateBinned(RooArgSet(*obs_var),nEvents,Extended(),Name(Form("%s_%s",it->first.c_str(),postfix.c_str())));
      else toyHist = it->second->generateBinned(RooArgSet(*obs_var),nEvents,Name(Form("%s_%s",it->first.c_str(),postfix.c_str())));
      toyDataHist.insert(pair<string,RooDataHist*>(toyHist->GetName(),toyHist));
      toy=toyHist;
    }
    else {
      RooDataSet *toySet;
      if (poisson) toySet = it->second->generate(RooArgSet(*obs_var),nEvents,Extended(),Name(Form("%s_%s",it->first.c_str(),postfix.c_str())));
      else toySet = it->second->generate(RooArgSet(*obs_var),nEvents,Name(Form("%s_%s",it->first.c_str(),postfix.c_str())));
      toyDataSet.insert(pair<string,RooDataSet*>(toySet->GetName(),toySet));
      toy=toySet;
    }
    toyData.insert(pair<string,RooAbsData*>(toy->GetName(),toy));
  }
  
}

map<string,RooAbsData*> PdfModelBuilder::getToyData(){
  return toyData;
}

map<string,RooAbsData*> PdfModelBuilder::getHybridToyData(){
  return toyHybridData;
}

void PdfModelBuilder::plotHybridToy(string prefix, int binning, vector<float> switchOverMasses, vector<string> functions, bool bkgOnly){

  map<string,RooAbsPdf*> pdfSet;
  if (bkgOnly) {
    pdfSet = bkgPdfs;
  }
  else {
    pdfSet = sbPdfs;
  }
  
  int tempColors[4] = {kBlue,kRed,kGreen+2,kMagenta};

  vector<string> cut_strings;
  cut_strings.push_back("cutstring0");
  obs_var->setRange("cutstring0",obs_var->getMin(),switchOverMasses[0]);
  for (unsigned int i=1; i<switchOverMasses.size(); i++){
    cut_strings.push_back(Form("cutstring%d",i));
    obs_var->setRange(Form("cutstring%d",i),switchOverMasses[i-1],switchOverMasses[i]);
  }
  cut_strings.push_back(Form("cutstring%d",int(switchOverMasses.size())));
  obs_var->setRange(Form("cutstring%d",int(switchOverMasses.size())),switchOverMasses[switchOverMasses.size()-1],obs_var->getMax());
  
  RooPlot *plot = obs_var->frame();
  TCanvas *canv = new TCanvas();
  int i=0;
  for (vector<string>::iterator func=functions.begin(); func!=functions.end(); func++){
    for (map<string,RooAbsPdf*>::iterator pdfIt = pdfSet.begin(); pdfIt != pdfSet.end(); pdfIt++){
      // check if in list of hybrid functions
      if (pdfIt->first.find(*func)!=string::npos) {
        for (map<string,RooAbsData*>::iterator toyIt = toyData.begin(); toyIt != toyData.end(); toyIt++){
          //cout << "pdf: " << pdfIt->first << " - toy: " << toyIt->first << endl; 
          if (toyIt->first.find(pdfIt->first)!=string::npos){
            RooAbsData *data = toyIt->second->reduce(CutRange(cut_strings[i].c_str()));
            data->plotOn(plot,Binning(binning),MarkerColor(tempColors[i]),LineColor(tempColors[i]),CutRange(cut_strings[i].c_str()));
            pdfIt->second->plotOn(plot,LineColor(tempColors[i]),Range(cut_strings[i].c_str()));
            i++;
          }
        }
      }
    }
  }
  for (map<string,RooAbsData*>::iterator hybrid=toyHybridData.begin(); hybrid!=toyHybridData.end(); hybrid++){
    hybrid->second->plotOn(plot,Binning(binning),MarkerSize(0.8),MarkerStyle(kFullSquare));
    plot->SetMinimum(0.0001);
    plot->Draw();
    canv->Print(Form("%s_%s.pdf",prefix.c_str(),hybrid->first.c_str()));
  }
  delete canv;
}

void PdfModelBuilder::plotToysWithPdfs(string prefix, int binning, bool bkgOnly){
  
  map<string,RooAbsPdf*> pdfSet;
  if (bkgOnly) {
    pdfSet = bkgPdfs;
  }
  else {
    pdfSet = sbPdfs;
  }
  TCanvas *canv = new TCanvas();
  for (map<string,RooAbsPdf*>::iterator pdfIt = pdfSet.begin(); pdfIt != pdfSet.end(); pdfIt++){
    for (map<string,RooAbsData*>::iterator toyIt = toyData.begin(); toyIt != toyData.end(); toyIt++){
      //cout << "pdf: " << pdfIt->first << " - toy: " << toyIt->first << endl; 
      if (toyIt->first.find(pdfIt->first)!=string::npos){
        RooPlot *plot = obs_var->frame();
        toyIt->second->plotOn(plot,Binning(binning));
        pdfIt->second->plotOn(plot,LineColor(kRed));
        pdfIt->second->paramOn(plot,LineColor(kRed),RooFit::Layout(0.34,0.96,0.89),RooFit::Format("NEA",AutoPrecision(1)));
        plot->Draw();
        canv->Print(Form("%s_%s.pdf",prefix.c_str(),pdfIt->first.c_str()));
        canv->Print(Form("%s_%s.png",prefix.c_str(),pdfIt->first.c_str()));
      }
    }
  }
  delete canv;

}

void PdfModelBuilder::saveWorkspace(string filename){
  
  TFile *outFile = new TFile(filename.c_str(),"RECREATE");
  outFile->cd();
  wsCache->Write();
  outFile->Close();
  delete outFile;
}

void PdfModelBuilder::saveWorkspace(TFile *file){
  file->cd();
  wsCache->Write();
}
