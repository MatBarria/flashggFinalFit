#ifndef FLASHGGFINALFIT_BACKGROUND_WSTFILEWRAPPER_H
#define FLASHGGFINALFIT_BACKGROUND_WSTFILEWRAPPER_H
#include "TFile.h"
#include "TObject.h"
#include "RooWorkspace.h"
#include "RooAbsPdf.h"
#include "RooAbsData.h"
#include "RooRealVar.h"
#include "RooCategory.h"
#include "RooArgSet.h"
#include <string>
#include <list>
#include <vector>

class WSTFileWrapper {
public:
  WSTFileWrapper(std::string, std::string);
  virtual ~WSTFileWrapper() {}
  RooRealVar*   var(std::string);
  RooAbsPdf*    pdf(std::string);
  RooCategory*  cat(std::string);
  RooAbsData*   data(std::string);

  std::list<RooAbsData*> allData();
  RooArgSet allVars();

  void Close();

private:
  std::vector<std::string> fnList;
  std::vector<TFile*> fileList;
  std::vector<RooWorkspace*> wsList;
};

#endif  // FLASHGGFINALFIT_BACKGROUND_WSTFILEWRAPPER_H