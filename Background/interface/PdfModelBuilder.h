#ifndef PdfModelBuilder_h 
#define PdfModelBuilder_h

#include <iostream>
#include <vector>
#include <string>
#include <map>

#include "RooRealVar.h"
#include "RooFormulaVar.h"
#include "RooAbsPdf.h"
#include "RooAbsData.h"
#include "RooDataSet.h"
#include "RooProduct.h"
#include "RooWorkspace.h"
#include "TFile.h"

using namespace std;
using namespace RooFit;

class PdfModelBuilder {
  
  public:
    PdfModelBuilder();
    ~PdfModelBuilder();

    void setObsVar(RooRealVar *var);
    void setSignalModifier(RooRealVar *var);
    void setSignalModifierVal(float val);
    void setSignalModifierConstant(bool val);
    void setKeysPdfAttributes(RooDataSet *data, double rho=2);

    void addBkgPdf(string type, int nParams, string name, bool cache=true);

    void setSignalPdf(RooAbsPdf *pdf, RooRealVar *norm=NULL);
    void setSignalPdfFromMC(RooDataSet *data);
    void makeSBPdfs(bool cache=true);

    map<string,RooAbsPdf*> getBkgPdfs();
    map<string,RooAbsPdf*> getSBPdfs();
    RooAbsPdf *getSigPdf();

    void fitToData(RooAbsData *data, bool bkgOnly=true, bool cache=true, bool print=false);
    void plotPdfsToData(RooAbsData *data, int binning, string name, bool bkgOnly=true, string specificPdfName="");
    void plotToysWithPdfs(string prefix, int binning, bool bkgOnly=true);
    void plotHybridToy(string prefix, int binning, vector<float> switchOverMasses, vector<string> functions, bool bkgOnly=true);
    
    void setSeed(int seed);
    void throwToy(string name, int nEvents, bool bkgOnly=true, bool binned=true, bool poisson=true, bool cache=true);
    RooDataSet *makeHybridDataset(vector<float> switchOverMasses, vector<RooDataSet*> dataForHybrid);
    void throwHybridToy(string name, int nEvents, vector<float> switchOverMasses, vector<string> functions, bool bkgOnly=true, bool binned=true, bool poisson=true, bool cache=true);
    map<string,RooAbsData*> getToyData();
    map<string,RooAbsData*> getHybridToyData();

    void saveWorkspace(TFile* file);
    void saveWorkspace(string filename);

    RooAbsPdf* getBernstein(string prefix, int order);
    RooAbsPdf* getChebychev(string prefix, int order);
    RooAbsPdf* getPowerLaw(string prefix, int order);
    RooAbsPdf* getPowerLawSingle(string prefix, int order);
    RooAbsPdf* getPowerLawGeneric(string prefix, int order);
    RooAbsPdf* getExponential(string prefix, int order);
    RooAbsPdf* getExponentialSingle(string prefix, int order);
    RooAbsPdf* getLaurentSeries(string prefix, int order);
    RooAbsPdf* getDYTheory(std::string prefix, int empOrder);
    RooAbsPdf* getDYBernstein(std::string prefix, int order);
    RooAbsPdf* getDYChebychev(std::string prefix, int order);
    RooAbsPdf* getDYPowerLaw(std::string prefix, int order);
    RooAbsPdf* getDYLaurent(string prefix, int order);
    RooAbsPdf* getDYBernBernstein(std::string prefix, int order);
    RooAbsPdf* getDYBernChebychev(std::string prefix, int order);
    RooAbsPdf* getDYBernPowerLaw(std::string prefix, int order);
    RooAbsPdf* getDYBernLaurent(string prefix, int order);
    RooAbsPdf* getDYExpChebychev(std::string prefix, int order, std::string cat = "");
    RooAbsPdf* getDYExpPowerLaw(std::string prefix, int order, std::string cat = "");
    RooAbsPdf* getDYExpLaurent(string prefix, int order, string cat = "");
    RooAbsPdf* getDYPowChebychev(std::string prefix, int order, std::string cat = "");
    RooAbsPdf* getDYPowPowerLaw(std::string prefix, int order, std::string cat = "");
    RooAbsPdf* getDYPowLaurent(string prefix, int order, string cat = "");
    RooAbsPdf* getDYFewzChebychev(std::string prefix, int order);
    RooAbsPdf* getDYFewzPowerLaw(std::string prefix, int order);
    RooAbsPdf* getDYFewzLaurent(string prefix, int order);    // MC corrected cores: fixed DYCore / DYSplineCore shape multiplied by a
    // per-category Chebyshev correction read from DYshapes/*.csv
    RooAbsPdf* getDYModChebychev(std::string prefix, int order, std::string cat = "");
    RooAbsPdf* getDYModPowerLaw(std::string prefix, int order, std::string cat = "");
    RooAbsPdf* getDYModLaurent(std::string prefix, int order, std::string cat = "");
    RooAbsPdf* getDYFewzModChebychev(std::string prefix, int order, std::string cat = "");
    RooAbsPdf* getDYFewzModPowerLaw(std::string prefix, int order, std::string cat = "");
    RooAbsPdf* getDYFewzModLaurent(std::string prefix, int order, std::string cat = "");
    RooAbsPdf* getBWZExpPoly(std::string prefix, int order);
    RooAbsPdf* getKeysPdf(string prefix);
    RooAbsPdf* getPdfFromFile(string &prefix);
    RooAbsPdf* getModBreitWigner(string prefix);
    RooAbsPdf* getBWZGamma(string prefix);
    RooAbsPdf* get2Exp(string prefix);
    RooAbsPdf* get2PowerLaw(string prefix);

    // Set the parameters of a 2Exp pdf to a starting configuration.
    // The 2Exp model is monotonically falling by construction (see get2Exp),
    // so these are simply different reproducible starting shapes, one per
    // 'attempt'.  Returns false if 'pdf' is not a 2Exp pdf.
    bool set2ExpMonotonicInitValues(RooAbsPdf *pdf, unsigned int attempt);

  private:
   
    vector<string> recognisedPdfTypes;

    map<string,RooAbsPdf*> bkgPdfs;
    map<string,RooAbsPdf*> sbPdfs;
    RooAbsPdf* sigPdf;
    RooAbsReal* sigNorm;
    RooRealVar *bkgYield;
    RooAbsReal *sigYield;
    RooDataSet *keysPdfData;
    double keysPdfRho;

    map<string,RooAbsData*> toyData;
    map<string,RooAbsData*> toyHybridData;
    map<string,RooDataSet*> toyDataSet;
    map<string,RooDataHist*> toyDataHist;

    map<string,RooRealVar*> params;
    map<string,RooFormulaVar*> prods;
    map<string,RooAbsPdf*> utilities;

    RooRealVar *obs_var;
    bool obs_var_set;
    RooRealVar *signalModifier;
    bool signal_modifier_set;
    bool signal_set;
    bool bkgHasFit;
    bool sbHasFit;
    bool keysPdfAttributesSet;
    vector<string> cut_strings;

    RooWorkspace *wsCache;

    int verbosity;

};
#endif
