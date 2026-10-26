// ============================================================
// fitDYshapes.cpp
//
// Unified producer of the four per-category DY background shape
// parameter files stored in Background/DYshapes/:
//
//   fit_results_2Exp.csv             two exponentials
//   fit_results_2PowerLaw.csv        two power laws
//   fit_results_DYCoreMod.csv        DYCore       * Chebyshev(3)
//   fit_results_DYSplineCoreMod.csv  DYSplineCore * Chebyshev(3)
//
// The first two models were ported from
//   /home/zguo/hmm_ana/fitMC_DYCore_categories.C
// the last two from the previous version of this file.
// Their column layouts are unchanged, so the readers in
// DYCore/src/DYCoreParameters.cc keep working as they are.
//
// Categories are discovered automatically from the input files:
//
//   <ROOT_IO>/tuples/split_tuples/ggH/output_bkg.root
//       trees ggh_bkg_ggHcatN  -> CSV rows named ggHcatN
//   <ROOT_IO>/tuples/split_tuples/VBF/output_bkg.root
//       trees vbf_bkg_VBFcatN  -> CSV rows named VBFcatN
//
// so ggH and VBF parameters live in the SAME four CSV files,
// one row per category.
//
// ROOT_IO is the variable exported by flashggFinalFit/setup.sh,
// therefore no path has to be edited in this file.  When ROOT_IO
// is not set, /home/zguo/HmmCombine/ntuples_v2d2d1 is used.
//
// Common settings (same for every model and every category):
//   fit range   110 - 150 GeV
//   binning     160 bins
//   input       weighted MC, branches CMS_hgg_mass / weight
//
// VBF categories have very low MC statistics, so isolated bins
// can fluctuate far above the smooth falling spectrum.  For VBF
// categories only, the content of the leftmost bin is taken as
// reference and every bin above 3 x that value is replaced by a
// linear interpolation of its neighbouring bins before fitting
// (see cleanVBFSpectrum).  The threshold can be changed with
// --vbf-maxratio and the treatment disabled with --no-vbf-clean.
//
// Usage (run from flashggFinalFit/Background):
//
//   ./bin/fitDYshapes [--ggH <file>] [--vbf <file>]
//                     [--cats a,b,c] [--outdir <dir>] [--tries <n>]
//                     [--no-plots] [--logy]
//
// Categories:
//   by default every tree "<prefix>ggHcatN" / "<prefix>VBFcatN" found
//   in the input files is fitted;  --cats restricts (and orders) the list.
//
// Defaults:
//   --outdir DYshapes      (relative to the working directory)
//   --tries  1             (one fit per model, as in the original scripts)
//   plots enabled: one figure per category with all four models,
//   written to <outdir>/fit_plots
// ============================================================

#include "TCanvas.h"
#include "TFile.h"
#include "TH1D.h"
#include "TKey.h"
#include "TLegend.h"
#include "TMath.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"

#include "RooAbsData.h"
#include "RooArgList.h"
#include "RooArgSet.h"
#include "RooCurve.h"
#include "RooDataHist.h"
#include "RooFitResult.h"
#include "RooFormulaVar.h"
#include "RooGenericPdf.h"
#include "RooHist.h"
#include "RooPlot.h"
#include "RooRandom.h"
#include "RooRealVar.h"

#include "flashggFinalFit/DYCore/interface/RooDYCore.h"
#include "flashggFinalFit/DYCore/interface/RooDYSplineCore.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace RooFit;

// ============================================================
// Common fit configuration
// ============================================================

static const double xmin = 110.0;
static const double xmax = 150.0;
static const int nBins = 160;

// number of fit attempts (--tries).  The default 1 reproduces the
// behaviour of the original scripts (a single fit).  Values >1 add
// random-restart retries, which is only advisable for the Chebyshev
// models: the 2Exp / 2PowerLaw likelihoods are quite flat and the
// restarts tend to end up on the parameter limits.
static int gMaxFitTries = 1;

// linear y axis for the validation plots (default).  A logarithmic
// axis can be switched on with --logy.
static bool gLogY = false;

// VBF outlier treatment: every VBF bin whose content exceeds
// gVBFOutlierRatio times the content of the leftmost bin is
// replaced by an interpolation of its neighbours.  A value <= 0
// disables the treatment (--no-vbf-clean).
static double gVBFOutlierRatio = 3.0;

// ============================================================
// Fit results
// ============================================================

struct ExpFitResult {
    double theta, theta_err;
    double a1, a1_err;
    double b1, b1_err;
    double c1, c1_err;
    double a2, a2_err;
    double frac, frac_err;
    int status;
    double edm;
};

struct PowFitResult {
    double frac, frac_err;
    double a1, a1_err;
    double b1, b1_err;
    double c1, c1_err;
    double a2, a2_err;
    int status;
    double edm;
};

struct ChebFitResult {
    double c1, c1_err;
    double c2, c2_err;
    double c3, c3_err;
    int status;
    double edm;
};

struct CsvOutputs {
    std::ofstream exp;
    std::ofstream pow;
    std::ofstream core;
    std::ofstream spline;
};

// everything that is needed later on for the validation plots
struct CategoryArtifacts {
    std::string cat;
    RooRealVar *mass;
    RooDataHist *data;
    RooAbsPdf *expPdf;
    RooAbsPdf *powPdf;
    RooAbsPdf *corePdf;
    RooAbsPdf *splinePdf;
    int expStatus;
    int powStatus;
    int coreStatus;
    int splineStatus;
};

// ============================================================
// Small helpers
// ============================================================

// trailing integer of a string, e.g. "VBFcat3" -> 3 ; -1 if none
static int trailingNumber(const std::string &s) {
    std::size_t i = s.size();
    while (i > 0 && std::isdigit(static_cast<unsigned char>(s[i - 1])))
        --i;
    if (i == s.size())
        return -1;
    return std::atoi(s.substr(i).c_str());
}

// collect the category names of all trees called "<prefix><cat>"
static std::vector<std::string>
discoverCategories(TFile *fin, const std::string &treePrefix) {
    std::vector<std::string> cats;

    if (!fin)
        return cats;

    TIter next(fin->GetListOfKeys());
    TKey *key = nullptr;

    while ((key = static_cast<TKey *>(next()))) {

        if (std::string(key->GetClassName()) != "TTree")
            continue;

        const std::string name = key->GetName();

        if (name.compare(0, treePrefix.size(), treePrefix) != 0)
            continue;

        const std::string cat = name.substr(treePrefix.size());
        if (cat.empty())
            continue;

        cats.push_back(cat);
    }

    std::sort(cats.begin(), cats.end(),
              [](const std::string &a, const std::string &b) {
                  const int na = trailingNumber(a);
                  const int nb = trailingNumber(b);
                  if (na != nb)
                      return na < nb;
                  return a < b;
              });

    cats.erase(std::unique(cats.begin(), cats.end()), cats.end());

    return cats;
}

// build the weighted MC histogram of one category
static TH1D *buildHistogram(TFile *fin, const std::string &treeName,
                            const std::string &cat) {
    TTree *tree = dynamic_cast<TTree *>(fin->Get(treeName.c_str()));

    if (!tree) {
        std::cerr << "ERROR: cannot find tree " << treeName << std::endl;
        return nullptr;
    }

    float diMuon_mass = 0.0;
    double weight = 1.0;

    tree->SetBranchAddress("CMS_hgg_mass", &diMuon_mass);
    tree->SetBranchAddress("weight", &weight);

    TH1D *h =
        new TH1D(Form("h_%s", cat.c_str()),
                 Form("%s MC background", cat.c_str()), nBins, xmin, xmax);
    h->Sumw2();

    const Long64_t nEntries = tree->GetEntries();

    double sumWeights = 0.0;
    Long64_t nSelected = 0;

    for (Long64_t i = 0; i < nEntries; ++i) {

        tree->GetEntry(i);

        if (diMuon_mass < xmin || diMuon_mass > xmax)
            continue;

        h->Fill(diMuon_mass, weight);
        sumWeights += weight;
        ++nSelected;
    }

    std::cout << "  tree " << treeName << " : entries = " << nEntries
              << " , selected = " << nSelected
              << " , weighted yield = " << sumWeights << std::endl;

    if (h->Integral() <= 0) {
        std::cerr << "WARNING: histogram for " << cat
                  << " has non-positive integral" << std::endl;
    }

    return h;
}

// ============================================================
// VBF spectrum cleaning
//
// The VBF categories have very low MC statistics: individual bins
// can fluctuate far above the smooth falling spectrum and then
// dominate (and bias) the fit.  The strategy, applied to VBF
// categories only, is:
//
//   * take the content of the leftmost bin of the fit range as
//     the reference value,
//   * a bin whose content exceeds ratio * reference (default
//     ratio = 3) is treated as an outlier,
//   * an outlier is replaced by a linear interpolation between
//     the closest non-outlier bins on its left and right.  A
//     RooDataHist cannot easily drop individual bins, so the bin
//     is kept in the fit but with a value consistent with its
//     neighbours.
//
// The bin errors are interpolated in the same way, which avoids
// giving the replaced bins an artificially small error.
// ============================================================

struct VBFCleanInfo {
    int referenceBin = -1;
    double referenceValue = 0.0;
    int nReplaced = 0;
};

static bool isVBFCategory(const std::string &cat) {
    return cat.compare(0, 3, "VBF") == 0;
}

static VBFCleanInfo cleanVBFSpectrum(TH1D *h, double ratio) {
    VBFCleanInfo info;

    if (!h || ratio <= 0.0)
        return info;

    const int nb = h->GetNbinsX();
    if (nb < 3)
        return info;

    // ---- reference value: the leftmost bin of the fit range ----
    int refBin = 1;
    if (!TMath::Finite(h->GetBinContent(refBin)) ||
        h->GetBinContent(refBin) <= 0.0) {

        refBin = -1;
        for (int i = 1; i <= nb; ++i) {
            const double v = h->GetBinContent(i);
            if (TMath::Finite(v) && v > 0.0) {
                refBin = i;
                break;
            }
        }

        if (refBin < 0) {
            std::cout << "  [VBF clean] no positive bin, nothing to do"
                      << std::endl;
            return info;
        }

        std::cout << "  [VBF clean] leftmost bin is not positive, using bin "
                  << refBin << " as reference" << std::endl;
    }

    const double ref = h->GetBinContent(refBin);
    const double threshold = ratio * ref;

    info.referenceBin = refBin;
    info.referenceValue = ref;

    // ---- flag the outliers (1-based bin indices) ----
    std::vector<char> flagged(nb + 2, 0);
    int nFlagged = 0;

    for (int i = 1; i <= nb; ++i) {
        const double v = h->GetBinContent(i);
        if (TMath::Finite(v) && v > threshold) {
            flagged[i] = 1;
            ++nFlagged;
        }
    }

    std::cout << "  [VBF clean] reference = bin " << refBin << " ("
              << h->GetXaxis()->GetBinCenter(refBin) << " GeV) = " << ref
              << " , threshold = " << ratio << " x " << ref << " = "
              << threshold << " , flagged bins = " << nFlagged << std::endl;

    if (nFlagged == 0)
        return info;

    // snapshot the original contents/errors, so that replacing one
    // bin does not influence the interpolation of the next one
    std::vector<double> val(nb + 2), err(nb + 2);
    for (int i = 1; i <= nb; ++i) {
        val[i] = h->GetBinContent(i);
        err[i] = h->GetBinError(i);
    }

    const TAxis *ax = h->GetXaxis();

    for (int i = 1; i <= nb; ++i) {

        if (!flagged[i])
            continue;

        // closest non-flagged bins on either side
        int left = i - 1;
        while (left >= 1 && flagged[left])
            --left;

        int right = i + 1;
        while (right <= nb && flagged[right])
            ++right;

        double newVal = 0.0;
        double newErr = 0.0;

        if (left >= 1 && right <= nb) {
            const double xl = ax->GetBinCenter(left);
            const double xm = ax->GetBinCenter(i);
            const double xr = ax->GetBinCenter(right);
            const double w = (xr > xl) ? (xm - xl) / (xr - xl) : 0.5;
            newVal = (1.0 - w) * val[left] + w * val[right];
            newErr = (1.0 - w) * err[left] + w * err[right];
        } else if (left >= 1) {
            newVal = val[left];
            newErr = err[left];
        } else if (right <= nb) {
            newVal = val[right];
            newErr = err[right];
        } else {
            continue;
        }

        if (!TMath::Finite(newVal) || newVal <= 0.0) {
            newVal = (left >= 1 && right <= nb)
                         ? std::min(val[left], val[right])
                         : (left >= 1 ? val[left] : val[right]);
        }

        // a non-positive interpolation would create a negative
        // expectation: leave the bin untouched in that case
        if (!TMath::Finite(newVal) || newVal <= 0.0)
            continue;

        if (!TMath::Finite(newErr) || newErr <= 0.0)
            newErr = std::sqrt(newVal);

        h->SetBinContent(i, newVal);
        h->SetBinError(i, newErr);

        ++info.nReplaced;

        std::cout << "  [VBF clean] bin " << i << " ("
                  << ax->GetBinCenter(i) << " GeV) : " << val[i] << " -> "
                  << newVal << "  (interpolated)" << std::endl;
    }

    std::cout << "  [VBF clean] replaced " << info.nReplaced << " of "
              << nFlagged << " flagged bin(s) for the fit" << std::endl;

    return info;
}

// ============================================================
// Fit with simple retries
//
// If the minimisation does not converge, restore the initial
// parameters, randomise them within their limits and try again
// (second and later attempts use strategy 2).  A result with
// status 0 is preferred, otherwise the one with the smallest
// NLL is kept.  The live parameters are left holding the
// values/errors of the returned fit.
// ============================================================

static RooFitResult *fitWithRetries(RooAbsPdf &pdf, RooAbsData &data) {
    const int maxTries = gMaxFitTries;

    RooArgSet *params = pdf.getParameters(data);

    RooArgSet initial;
    params->snapshot(initial);

    RooFitResult *best = nullptr;

    for (int itry = 0; itry < maxTries; ++itry) {

        RooFitResult *res = pdf.fitTo(
            data, Save(), PrintLevel(-1), SumW2Error(kTRUE),
            Minimizer("Minuit2", "migrad"), Strategy(itry == 0 ? 1 : 2));
        std::cout << "\n========================================\n";
        std::cout << "Fit: " << pdf.GetName() << "\n";
        std::cout << "========================================\n";

        std::cout << "Status     : " << res->status() << "\n";
        std::cout << "CovQual    : " << res->covQual() << "\n";
        std::cout << "EDM        : " << res->edm() << "\n";
        std::cout << "MinNLL     : " << res->minNll() << "\n";
        std::cout << "Invalid NLL: " << res->numInvalidNLL() << "\n";

        std::cout << "\nMinimization history:\n";

        for (unsigned int i = 0; i < res->numStatusHistory(); ++i) {

            std::cout << "  " << res->statusLabelHistory(i) << " : "
                      << res->statusCodeHistory(i) << "\n";
        }

        std::cout << "========================================\n";

        const bool better = (best == nullptr) ||
                            (res->status() == 0 && best->status() != 0) ||
                            ((res->status() == 0) == (best->status() == 0) &&
                             res->minNll() < best->minNll());

        if (better) {
            delete best;
            best = res;
        } else {
            delete res;
        }

        if (best->status() == 0)
            break;

        std::cout << "    fit status = " << best->status() << " , retrying ("
                  << (itry + 1) << "/" << maxTries << ")" << std::endl;

        params->assignValueOnly(initial);

        RooRandom::randomGenerator()->SetSeed(static_cast<UInt_t>(
            7919 * (itry + 1) + static_cast<int>(pdf.GetName()[0])));

        const RooArgList &random = best->randomizePars();

        bool finite = true;
        for (int i = 0; i < random.getSize(); ++i) {
            RooRealVar *v = dynamic_cast<RooRealVar *>(random.at(i));
            if (v && !TMath::Finite(v->getVal()))
                finite = false;
        }

        if (finite) {
            params->assignValueOnly(random);
        } else {
            params->assignValueOnly(initial);
        }
    }

    // make sure the caller sees the parameters of the returned fit
    const RooArgList &finalPars = best->floatParsFinal();

    for (int i = 0; i < finalPars.getSize(); ++i) {

        RooRealVar *v = dynamic_cast<RooRealVar *>(finalPars.at(i));
        if (!v)
            continue;

        RooRealVar *orig =
            dynamic_cast<RooRealVar *>(params->find(v->GetName()));
        if (!orig)
            continue;

        orig->setVal(v->getVal());
        orig->setError(v->getError());
    }

    delete params;

    return best;
}

// ============================================================
// Model 1 : two exponentials
//
//   x = (2*m-(xmin+xmax)) / (xmax-xmin)
//
//   f = frac * exp(a1*x + b1*x^2 + c1*x^3)
//       + (1-frac) * exp(a2*x)
//
//   frac = 1 / (1 + exp(-theta))
//
// ported from fitMC_DYCore_categories.C.  The normalised mass
// expression is written directly into the formula (instead of
// using an intermediate RooFormulaVar) so that the observable
// is an explicit dependency of the pdf, which RooPlot needs.
// ============================================================

static RooAbsPdf *buildAndFit2Exp(RooDataHist &data, RooRealVar &mass,
                                  const std::string &cat, ExpFitResult &out) {
    // The parameters are heap allocated on purpose: they are servers of
    // the returned pdf, which is used for the validation plots after this
    // function has returned.  They are left to ROOT at process exit.
    RooRealVar *exp_theta =
        new RooRealVar(Form("exp_theta_%s", cat.c_str()),
                       "logistic fraction parameter", 0.0, -7.0, 7.0);
    RooRealVar *exp_a1 = new RooRealVar(Form("exp_a1_%s", cat.c_str()),
                                        "linear1", 0.0, -15.0, 15.0);
    RooRealVar *exp_b1 = new RooRealVar(Form("exp_b1_%s", cat.c_str()), "quad1",
                                        0.0, -10.0, 10.0);
    RooRealVar *exp_c1 = new RooRealVar(Form("exp_c1_%s", cat.c_str()),
                                        "cubic1", 0.0, -4.0, 4.0);
    RooRealVar *exp_a2 = new RooRealVar(Form("exp_a2_%s", cat.c_str()),
                                        "linear2", 0.0, -15.0, 15.0);

    RooFormulaVar *exp_frac =
        new RooFormulaVar(Form("exp_frac_%s", cat.c_str()), "1/(1+exp(-@0))",
                          RooArgList(*exp_theta));

    const std::string x =
        Form("((2*@0-(%g+%g))/(%g-%g))", xmin, xmax, xmax, xmin);

    RooArgList expArgs;
    expArgs.add(mass);      // @0
    expArgs.add(*exp_frac); // @1
    expArgs.add(*exp_a1);   // @2
    expArgs.add(*exp_b1);   // @3
    expArgs.add(*exp_c1);   // @4
    expArgs.add(*exp_a2);   // @5

    const std::string formula = "@1*exp(@2*" + x + "+@3*pow(" + x + ",2)" +
                                "+@4*pow(" + x + ",3))" + "+(1-@1)*exp(@5*" +
                                x + ")";

    RooGenericPdf *expPdf = new RooGenericPdf(
        Form("MC_DYExpCore_%s", cat.c_str()), "MC fitted two exponential core",
        formula.c_str(), expArgs);

    RooFitResult *res = fitWithRetries(*expPdf, data);

    out.theta = exp_theta->getVal();
    out.theta_err = exp_theta->getError();

    out.a1 = exp_a1->getVal();
    out.a1_err = exp_a1->getError();

    out.b1 = exp_b1->getVal();
    out.b1_err = exp_b1->getError();

    out.c1 = exp_c1->getVal();
    out.c1_err = exp_c1->getError();

    out.a2 = exp_a2->getVal();
    out.a2_err = exp_a2->getError();

    out.frac = exp_frac->getVal();
    out.frac_err = exp_frac->getPropagatedError(*res);

    out.status = res->status();
    out.edm = res->edm();

    delete res;

    return expPdf;
}

// ============================================================
// Model 2 : two power laws
//
//   x = m / 125
//
//   f = frac * x^-(a1 + b1*x + c1*x^2)
//       + (1-frac) * x^-a2
//
// ported from fitMC_DYCore_categories.C
// ============================================================

static RooAbsPdf *buildAndFit2PowerLaw(RooDataHist &data, RooRealVar &mass,
                                       const std::string &cat,
                                       PowFitResult &out) {
    // heap allocated on purpose, see buildAndFit2Exp
    RooRealVar *pow_frac = new RooRealVar(Form("pow_frac_%s", cat.c_str()),
                                          "fraction", 0.95, 0.5, 1.0);
    RooRealVar *pow_a1 = new RooRealVar(Form("pow_a1_%s", cat.c_str()),
                                        "const1", 2.5, 0.0, 25.0);
    RooRealVar *pow_b1 = new RooRealVar(Form("pow_b1_%s", cat.c_str()),
                                        "linear1", 0.0, -15.0, 15.0);
    RooRealVar *pow_c1 = new RooRealVar(Form("pow_c1_%s", cat.c_str()), "quad1",
                                        0.0, -15.0, 15.0);
    RooRealVar *pow_a2 = new RooRealVar(Form("pow_a2_%s", cat.c_str()),
                                        "const2", 2.0, 0.0, 25.0);

    const std::string x = "(@0/125.0)";

    RooArgList powArgs;
    powArgs.add(mass);      // @0
    powArgs.add(*pow_frac); // @1
    powArgs.add(*pow_a1);   // @2
    powArgs.add(*pow_b1);   // @3
    powArgs.add(*pow_c1);   // @4
    powArgs.add(*pow_a2);   // @5

    const std::string formula = "@1*pow(" + x + ",-(@2+@3*" + x + "+@4*" + x +
                                "*" + x + "))" + "+(1-@1)*pow(" + x + ",-@5)";

    RooGenericPdf *powPdf = new RooGenericPdf(
        Form("MC_DYTPowCore_%s", cat.c_str()),
        "MC fitted three-power-plus-one core", formula.c_str(), powArgs);

    RooFitResult *res = fitWithRetries(*powPdf, data);

    out.frac = pow_frac->getVal();
    out.frac_err = pow_frac->getError();

    out.a1 = pow_a1->getVal();
    out.a1_err = pow_a1->getError();

    out.b1 = pow_b1->getVal();
    out.b1_err = pow_b1->getError();

    out.c1 = pow_c1->getVal();
    out.c1_err = pow_c1->getError();

    out.a2 = pow_a2->getVal();
    out.a2_err = pow_a2->getError();

    out.status = res->status();
    out.edm = res->edm();

    delete res;

    return powPdf;
}

// ============================================================
// Model 3 : fixed DY core * third order Chebyshev correction
//
//   f = core(m) * (1 + c1*T1(x) + c2*T2(x) + c3*T3(x))
//
//   x = (2*m-(xmin+xmax)) / (xmax-xmin)
//
// The constant term is absorbed in the normalisation, so only
// c1..c3 are floated.  The coefficients are read back at fit
// time by RooDYCoreMod / RooDYSplineCoreMod.
//
// Same structure as PdfModelBuilder::getDYModChebychev().
// ============================================================

static RooAbsPdf *buildAndFitChebCorrection(RooAbsPdf &core, RooDataHist &data,
                                            RooRealVar &mass,
                                            const std::string &tag,
                                            ChebFitResult &out) {
    // heap allocated on purpose, see buildAndFit2Exp
    RooRealVar *c1 =
        new RooRealVar(Form("c1_%s", tag.c_str()), "c1", 0.0, -2.0, 2.0);
    RooRealVar *c2 =
        new RooRealVar(Form("c2_%s", tag.c_str()), "c2", 0.0, -2.0, 2.0);
    RooRealVar *c3 =
        new RooRealVar(Form("c3_%s", tag.c_str()), "c3", 0.0, -2.0, 2.0);

    const std::string x =
        Form("(2*@1-(%g+%g))/(%g-%g)", xmin, xmax, xmax, xmin);

    RooArgList args;
    args.add(core); // @0
    args.add(mass); // @1
    args.add(*c1);  // @2
    args.add(*c2);  // @3
    args.add(*c3);  // @4

    const std::string formula = "@0*(1+@2*cos(acos(" + x +
                                "))"
                                "+@3*cos(2*acos(" +
                                x +
                                "))"
                                "+@4*cos(3*acos(" +
                                x + ")))";

    RooGenericPdf *pdf = new RooGenericPdf(Form("fit_%s", tag.c_str()),
                                           "core * (1 + c1*T1 + c2*T2 + c3*T3)",
                                           formula.c_str(), args);

    RooFitResult *res = fitWithRetries(*pdf, data);

    out.c1 = c1->getVal();
    out.c1_err = c1->getError();

    out.c2 = c2->getVal();
    out.c2_err = c2->getError();

    out.c3 = c3->getVal();
    out.c3_err = c3->getError();

    out.status = res->status();
    out.edm = res->edm();

    delete res;

    return pdf;
}

// ============================================================
// CSV headers / rows
//
// The column layouts must not change: they are read back by
// DYCore/src/DYCoreParameters.cc.
// ============================================================

static void writeHeaders(CsvOutputs &csv) {
    csv.exp << "cat,"
            << "theta,theta_err,"
            << "a1,a1_err,"
            << "b1,b1_err,"
            << "c1,c1_err,"
            << "a2,a2_err,"
            << "frac,frac_err,"
            << "fit_status,edm"
            << "\n";

    csv.pow << "cat,"
            << "frac,frac_err,"
            << "a1,a1_err,"
            << "b1,b1_err,"
            << "c1,c1_err,"
            << "a2,a2_err,"
            << "fit_status,edm"
            << "\n";

    csv.core << "cat,"
             << "c1,c1_err,"
             << "c2,c2_err,"
             << "c3,c3_err,"
             << "fit_status,edm"
             << "\n";

    csv.spline << "cat,"
               << "c1,c1_err,"
               << "c2,c2_err,"
               << "c3,c3_err,"
               << "fit_status,edm"
               << "\n";
}

static void writeExpRow(std::ofstream &csv, const std::string &cat,
                        const ExpFitResult &r) {
    csv << cat << "," << std::setprecision(12) << r.theta << "," << r.theta_err
        << "," << r.a1 << "," << r.a1_err << "," << r.b1 << "," << r.b1_err
        << "," << r.c1 << "," << r.c1_err << "," << r.a2 << "," << r.a2_err
        << "," << r.frac << "," << r.frac_err << "," << r.status << "," << r.edm
        << "\n";
    csv.flush();
}

static void writePowRow(std::ofstream &csv, const std::string &cat,
                        const PowFitResult &r) {
    csv << cat << "," << std::setprecision(12) << r.frac << "," << r.frac_err
        << "," << r.a1 << "," << r.a1_err << "," << r.b1 << "," << r.b1_err
        << "," << r.c1 << "," << r.c1_err << "," << r.a2 << "," << r.a2_err
        << "," << r.status << "," << r.edm << "\n";
    csv.flush();
}

static void writeChebRow(std::ofstream &csv, const std::string &cat,
                         const ChebFitResult &r) {
    csv << cat << "," << std::setprecision(12) << r.c1 << "," << r.c1_err << ","
        << r.c2 << "," << r.c2_err << "," << r.c3 << "," << r.c3_err << ","
        << r.status << "," << r.edm << "\n";
    csv.flush();
}

// ============================================================
// Validation plot
//
// One figure per category: the MC histogram together with the
// four fitted models on the same canvas.
//
// All curves are normalised to the total MC yield of the
// category, so they can be compared directly with the points.
// ============================================================

static void saveCategoryPlot(const CategoryArtifacts &art,
                             const std::string &path) {
    static int counter = 0;

    const double norm = art.data->sumEntries();

    const int colors[4] = {kRed, kBlue, kGreen + 2, kMagenta + 1};
    const int styles[4] = {1, 2, 7, 9};
    const char *labels[4] = {"2 Exp", "2 Power law",
                             "DYCore #times Chebyshev(3)",
                             "DYSplineCore #times Chebyshev(3)"};
    const int status[4] = {art.expStatus, art.powStatus, art.coreStatus,
                           art.splineStatus};
    RooAbsPdf *pdfs[4] = {art.expPdf, art.powPdf, art.corePdf, art.splinePdf};

    TCanvas canv(Form("canv_%d", counter++), "canv", 900, 800);
    canv.SetLeftMargin(0.13);
    canv.SetRightMargin(0.05);
    canv.SetTopMargin(0.08);

    RooPlot *frame =
        art.mass->frame(Title(Form("%s MC background", art.cat.c_str())));

    art.data->plotOn(frame, Name("data"), DataError(RooAbsData::SumW2),
                     MarkerStyle(20), MarkerSize(0.9), LineColor(kBlack));

    for (int i = 0; i < 4; ++i) {
        pdfs[i]->plotOn(frame, Name(Form("curve%d", i)), LineColor(colors[i]),
                        LineStyle(styles[i]), LineWidth(3),
                        Normalization(norm, RooAbsReal::NumEvent));
    }

    frame->GetYaxis()->SetTitle(Form("Events / %g GeV", (xmax - xmin) / nBins));
    frame->GetYaxis()->SetTitleOffset(1.5);

    if (gLogY) {
        canv.SetLogy();
        frame->SetMinimum(0.5);
    } else {
        frame->SetMinimum(0.0);
    }

    frame->Draw();

    // these spectra fall with increasing mass, so the lower part of
    // the frame is the free region for the legend
    TLegend leg(0.42, 0.13, 0.94, 0.46);
    leg.SetBorderSize(0);
    leg.SetFillStyle(0);
    leg.SetTextSize(0.028);

    leg.AddEntry(frame->getHist("data"), "MC background", "lep");

    for (int i = 0; i < 4; ++i) {
        leg.AddEntry(frame->getCurve(Form("curve%d", i)),
                     Form("%s   (status %d)", labels[i], status[i]), "l");
    }

    leg.Draw("same");

    canv.SaveAs((path + ".png").c_str());
    canv.SaveAs((path + ".pdf").c_str());

    delete frame;
}

// ============================================================
// One category: all four models
//
// The four CSV rows are written (and flushed) here, the pdf
// objects needed for the validation plots are handed back to
// the caller.  They are intentionally never deleted: the
// process is short lived and ROOT keeps track of them.
// ============================================================

static void processCategory(TFile *fin, const std::string &treeName,
                            const std::string &cat, CsvOutputs &csv,
                            std::vector<CategoryArtifacts> &artifacts,
                            std::vector<std::string> &summary) {
    std::cout << "\n####################################################"
              << std::endl;
    std::cout << " Category " << cat << "   (tree " << treeName << ")"
              << std::endl;
    std::cout << "####################################################"
              << std::endl;

    TH1D *h = buildHistogram(fin, treeName, cat);

    if (!h) {
        std::cerr << "WARNING: no histogram for " << cat << " , skipping"
                  << std::endl;
        return;
    }

    if (h->Integral() <= 0) {
        std::cerr << "WARNING: non-positive integral for " << cat
                  << " , skipping" << std::endl;
        delete h;
        return;
    }

    // VBF only: replace isolated high bins so that they do not
    // bias the fit (the histogram is still the one entering the
    // RooDataHist, hence the fit and the validation plots see
    // the interpolated values)
    if (isVBFCategory(cat) && gVBFOutlierRatio > 0.0)
        cleanVBFSpectrum(h, gVBFOutlierRatio);

    CategoryArtifacts art;
    art.cat = cat;
    art.mass = new RooRealVar(Form("CMS_hmm_mass_%s", cat.c_str()),
                              "m_{#mu#mu}", xmin, xmax);
    art.data =
        new RooDataHist(Form("data_%s", cat.c_str()),
                        Form("data_%s", cat.c_str()), RooArgList(*art.mass), h);

    delete h; // RooDataHist copies the contents

    // --------------------------------------------------------
    // 2 Exp
    // --------------------------------------------------------
    std::cout << "\n--- " << cat << " : 2Exp ---" << std::endl;

    ExpFitResult expRes;
    art.expPdf = buildAndFit2Exp(*art.data, *art.mass, cat, expRes);

    std::cout << std::setprecision(12) << "  theta = " << expRes.theta
              << " +/- " << expRes.theta_err << "\n"
              << "  a1    = " << expRes.a1 << " +/- " << expRes.a1_err << "\n"
              << "  b1    = " << expRes.b1 << " +/- " << expRes.b1_err << "\n"
              << "  c1    = " << expRes.c1 << " +/- " << expRes.c1_err << "\n"
              << "  a2    = " << expRes.a2 << " +/- " << expRes.a2_err << "\n"
              << "  frac  = " << expRes.frac << " +/- " << expRes.frac_err
              << "\n"
              << "  status = " << expRes.status << " , edm = " << expRes.edm
              << std::endl;

    writeExpRow(csv.exp, cat, expRes);

    // --------------------------------------------------------
    // 2 Power Law
    // --------------------------------------------------------
    std::cout << "\n--- " << cat << " : 2PowerLaw ---" << std::endl;

    PowFitResult powRes;
    art.powPdf = buildAndFit2PowerLaw(*art.data, *art.mass, cat, powRes);

    std::cout << std::setprecision(12) << "  frac = " << powRes.frac << " +/- "
              << powRes.frac_err << "\n"
              << "  a1   = " << powRes.a1 << " +/- " << powRes.a1_err << "\n"
              << "  b1   = " << powRes.b1 << " +/- " << powRes.b1_err << "\n"
              << "  c1   = " << powRes.c1 << " +/- " << powRes.c1_err << "\n"
              << "  a2   = " << powRes.a2 << " +/- " << powRes.a2_err << "\n"
              << "  status = " << powRes.status << " , edm = " << powRes.edm
              << std::endl;

    writePowRow(csv.pow, cat, powRes);

    // --------------------------------------------------------
    // DYCore * Chebyshev(3)
    // --------------------------------------------------------
    std::cout << "\n--- " << cat << " : DYCore * Chebyshev(3) ---" << std::endl;

    ChebFitResult dyChebRes;
    {
        RooAbsPdf *core = new RooDYCore(Form("core_dy_%s", cat.c_str()),
                                        "fixed DY core", *art.mass);

        art.corePdf = buildAndFitChebCorrection(*core, *art.data, *art.mass,
                                                cat + "_dycore", dyChebRes);

        std::cout << std::setprecision(12) << "  c1 = " << dyChebRes.c1
                  << " +/- " << dyChebRes.c1_err << "\n"
                  << "  c2 = " << dyChebRes.c2 << " +/- " << dyChebRes.c2_err
                  << "\n"
                  << "  c3 = " << dyChebRes.c3 << " +/- " << dyChebRes.c3_err
                  << "\n"
                  << "  status = " << dyChebRes.status
                  << " , edm = " << dyChebRes.edm << std::endl;

        writeChebRow(csv.core, cat, dyChebRes);
    }

    // --------------------------------------------------------
    // DYSplineCore * Chebyshev(3)
    // --------------------------------------------------------
    std::cout << "\n--- " << cat << " : DYSplineCore * Chebyshev(3) ---"
              << std::endl;

    ChebFitResult splineChebRes;
    {
        RooAbsPdf *core =
            new RooDYSplineCore(Form("core_spline_%s", cat.c_str()),
                                "fixed DY spline core", *art.mass);

        art.splinePdf = buildAndFitChebCorrection(
            *core, *art.data, *art.mass, cat + "_spline", splineChebRes);

        std::cout << std::setprecision(12) << "  c1 = " << splineChebRes.c1
                  << " +/- " << splineChebRes.c1_err << "\n"
                  << "  c2 = " << splineChebRes.c2 << " +/- "
                  << splineChebRes.c2_err << "\n"
                  << "  c3 = " << splineChebRes.c3 << " +/- "
                  << splineChebRes.c3_err << "\n"
                  << "  status = " << splineChebRes.status
                  << " , edm = " << splineChebRes.edm << std::endl;

        writeChebRow(csv.spline, cat, splineChebRes);
    }

    // summary line
    char line[256];
    snprintf(line, sizeof(line),
             "  %-10s  2Exp %3d | 2Pow %3d | DYCoreMod %3d | DYSplineMod %3d",
             cat.c_str(), expRes.status, powRes.status, dyChebRes.status,
             splineChebRes.status);
    summary.push_back(line);

    art.expStatus = expRes.status;
    art.powStatus = powRes.status;
    art.coreStatus = dyChebRes.status;
    art.splineStatus = splineChebRes.status;

    artifacts.push_back(art);
}

// ============================================================
// Usage
// ============================================================

static void printUsage(const char *exe) {
    std::cout
        << "Usage: " << exe << " [options]\n"
        << "\n"
        << "  --ggH   <file>   ggH MC background file\n"
        << "                   default: "
           "$ROOT_IO/tuples/split_tuples/ggH/output_bkg.root\n"
        << "  --vbf   <file>   VBF MC background file\n"
        << "                   default: "
           "$ROOT_IO/tuples/split_tuples/VBF/output_bkg.root\n"
        << "  --cats  <list>   comma separated categories to fit, e.g.\n"
        << "                   ggHcat1,ggHcat3,VBFcat2\n"
        << "                   default: every category found in the input "
           "files\n"
        << "  --outdir <dir>   output directory (default: DYshapes)\n"
        << "  --tries  <n>     number of fit attempts per model (default: 1,\n"
        << "                   which reproduces the original single-fit "
           "scripts)\n"
        << "  --no-plots       skip the validation plots\n"
        << "  --logy           logarithmic y axis for the validation plots\n"
        << "                   (default: linear)\n"
        << "  --vbf-maxratio <r>\n"
        << "                   VBF only: bins above r x (leftmost bin) are\n"
        << "                   replaced by an interpolation of their\n"
        << "                   neighbours before the fit (default: 3)\n"
        << "  --no-vbf-clean   disable the VBF outlier treatment\n"
        << "  -h, --help       this message\n"
        << "\n"
        << "ROOT_IO is the variable exported by flashggFinalFit/setup.sh.\n"
        << std::endl;
}

// ============================================================
// Main
// ============================================================

int main(int argc, char *argv[]) {
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(0);

    // --------------------------------------------------------
    // input files : driven by ROOT_IO (setup.sh)
    // --------------------------------------------------------
    const char *envRootIo = std::getenv("ROOT_IO");

    if (!envRootIo || envRootIo[0] == '\0') {
        std::cerr << "ERROR: ROOT_IO is not set. Source setup.sh first."
                  << std::endl;
        return 1;
    }

    std::string rootIo = envRootIo;

    while (!rootIo.empty() && rootIo.back() == '/') {
        rootIo.pop_back();
    }

    std::string ggHFile = rootIo + "/tuples/split_tuples/ggH/output_bkg.root";
    std::string vbfFile = rootIo + "/tuples/split_tuples/VBF/output_bkg.root";
    std::string outDir = "DYshapes";
    std::string catsArg = "";

    bool doPlots = true;

    for (int i = 1; i < argc; ++i) {

        const std::string arg = argv[i];

        if (arg == "--ggH" && i + 1 < argc) {
            ggHFile = argv[++i];
        } else if (arg == "--vbf" && i + 1 < argc) {
            vbfFile = argv[++i];
        } else if (arg == "--outdir" && i + 1 < argc) {
            outDir = argv[++i];
        } else if (arg == "--cats" && i + 1 < argc) {
            catsArg = argv[++i];
        } else if (arg == "--tries" && i + 1 < argc) {
            gMaxFitTries = std::atoi(argv[++i]);
            if (gMaxFitTries < 1)
                gMaxFitTries = 1;
        } else if (arg == "--no-plots") {
            doPlots = false;
        } else if (arg == "--vbf-maxratio" && i + 1 < argc) {
            gVBFOutlierRatio = std::atof(argv[++i]);
            if (gVBFOutlierRatio < 0.0)
                gVBFOutlierRatio = 0.0;
        } else if (arg == "--no-vbf-clean") {
            gVBFOutlierRatio = 0.0;
        } else if (arg == "--logy") {
            gLogY = true;
        } else if (arg == "--liny") {
            gLogY = false;
        } else if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "ERROR: unknown option '" << arg << "'" << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    // --------------------------------------------------------
    // output
    // --------------------------------------------------------
    gSystem->mkdir(outDir.c_str(), kTRUE);

    const std::string plotDir = outDir + "/fit_plots";
    if (doPlots)
        gSystem->mkdir(plotDir.c_str(), kTRUE);

    CsvOutputs csv;
    csv.exp.open((outDir + "/fit_results_2Exp.csv").c_str());
    csv.pow.open((outDir + "/fit_results_2PowerLaw.csv").c_str());
    csv.core.open((outDir + "/fit_results_DYCoreMod.csv").c_str());
    csv.spline.open((outDir + "/fit_results_DYSplineCoreMod.csv").c_str());

    if (!csv.exp || !csv.pow || !csv.core || !csv.spline) {
        std::cerr << "ERROR: cannot create the output CSV files in " << outDir
                  << std::endl;
        return 1;
    }

    writeHeaders(csv);

    std::cout << "===================================================="
              << std::endl;
    std::cout << " Fitting per-category DY background shapes" << std::endl;
    std::cout << " ROOT_IO    : " << rootIo << std::endl;
    std::cout << " ggH input  : " << ggHFile << std::endl;
    std::cout << " VBF input  : " << vbfFile << std::endl;
    std::cout << " Output dir : " << outDir << std::endl;
    std::cout << " Categories : "
              << (catsArg.empty() ? std::string("auto (from the input files)")
                                  : catsArg)
              << std::endl;
    std::cout << " Fit range  : " << xmin << " - " << xmax << " GeV , " << nBins
              << " bins" << std::endl;
    std::cout << " VBF clean  : "
              << (gVBFOutlierRatio > 0.0
                      ? Form("bins > %g x leftmost replaced by interpolation",
                             gVBFOutlierRatio)
                      : std::string("disabled"))
              << std::endl;
    std::cout << "===================================================="
              << std::endl;

    struct Input {
        std::string label;
        std::string file;
        std::string treePrefix;
    };

    const std::vector<Input> inputs = {
        {"ggH", ggHFile, "ggh_bkg_"},
        {"VBF", vbfFile, "vbf_bkg_"},
    };

    std::vector<CategoryArtifacts> artifacts;
    std::vector<std::string> summary;
    int nMissing = 0;

    // --------------------------------------------------------
    // open the inputs and build the list of categories to fit
    //
    //   default      : everything found in the files
    //   --cats a,b,c : only these categories, in the given order
    // --------------------------------------------------------
    std::vector<TFile *> files;
    files.reserve(inputs.size());

    for (const Input &in : inputs) {

        TFile *fin = TFile::Open(in.file.c_str());

        if (!fin || fin->IsZombie()) {
            std::cerr << "ERROR: cannot open " << in.label << " input file "
                      << in.file << std::endl;
            ++nMissing;
        }

        files.push_back(fin);
    }

    std::vector<std::pair<int, std::string>>
        workList; // (input index, category)

    if (catsArg.empty()) {

        for (std::size_t i = 0; i < inputs.size(); ++i) {

            if (!files[i])
                continue;

            const std::vector<std::string> cats =
                discoverCategories(files[i], inputs[i].treePrefix);

            std::cout << "\n[INFO] " << inputs[i].label << " : found "
                      << cats.size() << " categories in " << inputs[i].file
                      << std::endl;

            if (cats.empty()) {
                std::cerr << "WARNING: no tree with prefix '"
                          << inputs[i].treePrefix << "' in " << inputs[i].file
                          << std::endl;
            }

            for (const std::string &cat : cats) {
                workList.push_back(std::make_pair(static_cast<int>(i), cat));
            }
        }

    } else {

        std::vector<std::string> requested;
        {
            std::stringstream ss(catsArg);
            std::string item;
            while (std::getline(ss, item, ',')) {
                if (!item.empty())
                    requested.push_back(item);
            }
        }

        std::cout << "\n[INFO] using the --cats list (" << requested.size()
                  << " entries)" << std::endl;

        for (const std::string &cat : requested) {

            int found = -1;

            for (std::size_t i = 0; i < inputs.size(); ++i) {
                if (!files[i])
                    continue;
                if (files[i]->Get((inputs[i].treePrefix + cat).c_str())) {
                    found = static_cast<int>(i);
                    break;
                }
            }

            if (found < 0) {
                std::cerr << "WARNING: no tree for category '" << cat
                          << "' in any input file, skipping" << std::endl;
                continue;
            }

            workList.push_back(std::make_pair(found, cat));
        }
    }

    if (workList.empty()) {
        std::cerr << "ERROR: no category to fit." << std::endl;
        return 1;
    }

    // --------------------------------------------------------
    // fits (all CSV rows are written here)
    // --------------------------------------------------------
    for (const std::pair<int, std::string> &job : workList) {
        processCategory(files[job.first],
                        inputs[job.first].treePrefix + job.second, job.second,
                        csv, artifacts, summary);
    }

    for (TFile *fin : files) {
        if (fin)
            fin->Close();
    }

    csv.exp.close();
    csv.pow.close();
    csv.core.close();
    csv.spline.close();

    // --------------------------------------------------------
    // validation plots (separate pass: a plotting problem can
    // no longer truncate the CSV files)
    //
    // one figure per category with the four models overlaid
    // --------------------------------------------------------
    if (doPlots) {

        std::cout << "\n=== Writing one plot per category to " << plotDir
                  << " ===" << std::endl;

        for (const CategoryArtifacts &art : artifacts) {
            saveCategoryPlot(art,
                             Form("%s/%s", plotDir.c_str(), art.cat.c_str()));
        }
    }

    // --------------------------------------------------------
    // report
    // --------------------------------------------------------
    std::cout << "\n===================================================="
              << std::endl;
    std::cout << " Summary" << std::endl;
    std::cout << "===================================================="
              << std::endl;

    for (const std::string &line : summary) {
        std::cout << line << std::endl;
    }

    std::cout << "\nWritten in " << outDir << " :" << std::endl;
    std::cout << "  fit_results_2Exp.csv" << std::endl;
    std::cout << "  fit_results_2PowerLaw.csv" << std::endl;
    std::cout << "  fit_results_DYCoreMod.csv" << std::endl;
    std::cout << "  fit_results_DYSplineCoreMod.csv" << std::endl;

    if (nMissing > 0) {
        std::cerr << "\nERROR: " << nMissing
                  << " input file(s) could not be opened." << std::endl;
        return 1;
    }

    return 0;
}
