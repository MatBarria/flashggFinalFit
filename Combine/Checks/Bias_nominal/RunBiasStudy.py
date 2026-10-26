#!/usr/bin/env python
import os
import csv

from biasUtils import *

from optparse import OptionParser
parser = OptionParser()
parser.add_option("-d","--datacard",default="Datacard.root")
parser.add_option("-w","--workspace",default="w")
parser.add_option("-t","--toys",action="store_true", default=False)
parser.add_option("-n","--nToys",default=500,type="int")
parser.add_option("-f","--fits",action="store_true", default=False)
parser.add_option("-p","--plots",action="store_true", default=False)
parser.add_option("-e","--expectSignal",default=1.,type="float")
parser.add_option("-m","--mH",default=125.,type="float")
parser.add_option("-c","--combineOptions",default="")
parser.add_option("-s","--seed",default=-1,type="int")
parser.add_option("--dryRun",action="store_true", default=False)
parser.add_option("--poi",default="r")
parser.add_option("--split",default=500,type="int")
parser.add_option("--selectFunction",default=None)
parser.add_option("--gaussianFit",action="store_true", default=False)
parser.add_option("-b", "--BDTCategory", default="")
parser.add_option("--generateCSV",action="store_true", default=False)
parser.add_option("--outCSV", dest="outCSV", default="bias_results.csv", help="Filename for the yields output CSV file")

(opts,args) = parser.parse_args()
#print()
if opts.nToys>opts.split and not opts.nToys%opts.split==0: raise RuntimeError('The number of toys %g needs to be smaller than or divisible by the split number %g'%(opts.nToys, opts.split))


import ROOT as r
r.gROOT.SetBatch(True)
r.gStyle.SetOptStat(2211)

ws = r.TFile(opts.datacard).Get(opts.workspace)

pdfs = rooArgSetToList(ws.allPdfs())
multipdfName = None
for pdf in pdfs:
    if pdf.InheritsFrom("RooMultiPdf"):
        if multipdfName is not None: raiseMultiError() 
        multipdfName = pdf.GetName()
        print('Conduct bias study for multipdf called %s'%multipdfName)

multipdf = ws.pdf(multipdfName)
#print(multipdf)

varlist = rooArgSetToList(ws.allCats())
indexName = None
for var in varlist:
    if var.GetName().startswith('pdfindex'):
        if indexName is not None: raiseMultiError()
        indexName = var.GetName()
        print('Found index called %s'%indexName)
print()

from collections import OrderedDict as od
indexNameMap = od()
for ipdf in range(multipdf.getNumPdfs()):
    if opts.selectFunction is not None:
        if not multipdf.getPdf(ipdf).GetName().count(opts.selectFunction): continue
    indexNameMap[ipdf] = multipdf.getPdf(ipdf).GetName()

if opts.toys:
    print("\nCreating toys")
    if not os.path.isdir('BiasToys'): os.system('mkdir -p BiasToys')
    os.system('rm -f BiasToys/*.root')
    toyCmdBase = 'combine -m %.4f -d %s -M GenerateOnly --expectSignal %.4f -s %g --saveToys'%(opts.mH, opts.datacard, opts.expectSignal, opts.seed)
    #print("Base toy command:")
    #print(toyCmdBase)
    #print()
    #quit()
    for ipdf,pdfName in indexNameMap.items():
        name = shortName(pdfName)
        print("Creating toys for: " + name)
        if opts.nToys > opts.split:
            for isplit in range(opts.nToys//opts.split):
                toyCmd = toyCmdBase + ' -t %g -n _%s_split%g --setParameters %s=%g,r=1 --freezeParameters %s'%(opts.split, name, isplit, indexName, ipdf, indexName)
                print("Run toy command:")
                #print(toyCmd + "\n")
                run(toyCmd, dry=opts.dryRun)
                print('mv higgsCombine_%s* %s'%(name, toyName(name,opts.BDTCategory,split=isplit)))
                os.system('mv higgsCombine_%s* %s'%(name, toyName(name,opts.BDTCategory,split=isplit)))
        else: 
            toyCmd = toyCmdBase + ' -t %g -n _%s --setParameters %s=%g,r=1 --freezeParameters %s'%(opts.nToys, name, indexName, ipdf, indexName)
            print("Run toy command:")
            #print(toyCmd + "\n")
            run(toyCmd, dry=opts.dryRun)
            print('mv higgsCombine_%s* %s'%(name, toyName(name, opts.BDTCategory)))
            os.system('mv higgsCombine_%s* %s'%(name, toyName(name, opts.BDTCategory)))
            
print()

if opts.fits:
    if not os.path.isdir('BiasFits'): os.system('mkdir -p BiasFits')
    os.system('rm -f BiasFits/*.root')
    fitCmdBase = 'combine -m %.4f -d %s -M MultiDimFit -P %s --algo singles %s --robustFit 1'%(opts.mH, opts.datacard, opts.poi, opts.combineOptions)
    #print("Base fits command:")
    #print(fitCmdBase)
    #quit()
    for ipdf,pdfName in indexNameMap.items():
        name = shortName(pdfName)
        print("Fitting for toy: " + name)
        for fitipdf, fitpdfName in indexNameMap.items():
            fitFxnName = shortName(fitpdfName)
            print("Fitting toy: " + name + " wth pdf model: " + fitFxnName)
            if opts.nToys > opts.split:
                for isplit in range(opts.nToys//opts.split):
                    fitCmd = fitCmdBase + ' -t %g -n _%s_split%g --toysFile=%s --setParameters %s=%g --freezeParameters MH,%s'%(opts.split, name + "toy_" + fitFxnName + "fit", isplit, toyName(name,opts.BDTCategory, split=isplit), indexName, fitipdf, indexName)
                    print("Run fits command:")
                    run(fitCmd, dry=opts.dryRun)
                    print('mv higgsCombine_%s* %s'%(name, fitName(name,fitFxnName, opts.BDTCategory, split=isplit)))
                    os.system('mv higgsCombine_%s* %s'%(name, fitName(name,fitFxnName, opts.BDTCategory, split=isplit)))
                run('hadd %s BiasFits/*%s*%s*split*%s*.root'%(fitName(name, fitFxnName, opts.BDTCategory), opts.BDTCategory, name, fitFxnName), dry=opts.dryRun)
            else:
                fitCmd = fitCmdBase + ' -t %g -n _%s --toysFile=%s --setParameters %s=%g,r=1 --freezeParameters MH,%s'%(opts.nToys, name + "toy_" + fitFxnName + "fit", toyName(name, opts.BDTCategory), indexName, fitipdf, indexName)
                print("Run fits command:")
                #print(fitCmd + "\n")
                #quit()
                run(fitCmd, dry=opts.dryRun)
                print('mv higgsCombine_%s* %s'%(name, fitName(name, fitFxnName, opts.BDTCategory)))
                os.system('mv higgsCombine_%s* %s'%(name, fitName(name, fitFxnName, opts.BDTCategory)))

if opts.plots:
    if not os.path.isdir('BiasPlots'): os.system('mkdir -p BiasPlots')
    os.system('rm -f BiasPlots/*.root')
    for ipdf,pdfName in indexNameMap.items():
        name = shortName(pdfName)
        for fitipdf, fitpdfName in indexNameMap.items():
            fitFxnName = shortName(fitpdfName)
            tfile = r.TFile(fitName(name, fitFxnName, opts.BDTCategory))
            if not tfile or tfile.IsZombie():
                print("Missing/bad fit file:", fitName(name, fitFxnName, opts.BDTCategory))
                continue

            tree = tfile.Get("limit")
            if not tree:
                print("No limit tree in:", fitName(name, fitFxnName, opts.BDTCategory))
                continue

            print(name, fitFxnName, "entries =", tree.GetEntries())
            if tree.GetEntries() == 0:
                print("No fit entries:", fitName(name, fitFxnName, opts.BDTCategory))
                continue

            print('pullsForTruth_%s'%name, '%s pull distribution using %s to fit %s toys'%(opts.BDTCategory,fitFxnName,name))
            pullHist = r.TH1F('pullsForTruth_%s'%name, '%s pull distribution using %s to fit %s toys'%(opts.BDTCategory,fitFxnName,name), 160, -8., 8.)
            pullHist.GetXaxis().SetTitle('Pull')
            pullHist.GetYaxis().SetTitle('Entries')
            for itoy in range(opts.nToys):
                tree.GetEntry(3*itoy)
                if not getattr(tree,'quantileExpected')==-1: 
                    raiseFailError(itoy,True) 
                    continue
                bf = getattr(tree, 'r')
                tree.GetEntry(3*itoy+1)
                if not abs(getattr(tree,'quantileExpected')--0.32)<0.001: 
                    raiseFailError(itoy,True) 
                    continue
                lo = getattr(tree, 'r')
                tree.GetEntry(3*itoy+2)
                if not abs(getattr(tree,'quantileExpected')-0.32)<0.001: 
                    raiseFailError(itoy,True) 
                    continue
                hi = getattr(tree, 'r')
                diff = bf - opts.expectSignal
                unc = 0.5 * (hi-lo)
                if unc > 0.: 
                    pullHist.Fill(diff/unc)
                canv = r.TCanvas()
                pullHist.Draw()
            if opts.gaussianFit:
                r.gStyle.SetOptFit(111)
                fit_result = pullHist.Fit('gaus', "S")
                fit = pullHist.GetFunction('gaus')
                canv.SaveAs('%s.pdf'%plotName(name, fitFxnName, opts.BDTCategory))
                canv.SaveAs('%s.png'%plotName(name, fitFxnName, opts.BDTCategory))

                fit_status = int(fit_result)
                fit_valid = 0

                #if fit_result:
                #    fit_status = int(fit_result.Status())

                if fit and fit_status == 0:
                    bias_pull = fit.GetParameter(1)
                    bias_pull_err = fit.GetParError(1)
                    fit_sigma = fit.GetParameter(2)
                    fit_sigma_err = fit.GetParError(2)
                    fit_chi2 = fit.GetChisquare()
                    fit_valid = 1
                else:
                    print("WARNING: Gaussian fit failed for: %s, %s, %s"%(name, fitFxnName, opts.BDTCategory))
                    print("Entries in pullHist: ", pullHist.GetEntries())
                    print("Fit status: ", fit_status)

                    bias_pull = float("nan")
                    bias_pull_err = float("nan")
                    fit_sigma = float("nan")
                    fit_sigma_err = float("nan")
                    fit_chi2 = float("nan")
                
            if opts.generateCSV:
                ##CSV Fields
                fields = [
                    "category",
                    "toy_function",
                    "fit_function",
                    "n_toys",
                    "bias_pull",
                    "bias_pull_err",
                    "fit_sigma",
                    "fit_sigma_err",
                    "fit_chi2",
                    "fit_valid",
                ]
                cat = opts.BDTCategory
                print("cat: " + cat)

                csv_exists = os.path.isfile(opts.outCSV)

                with open(opts.outCSV, mode='a', newline='') as f:
                    writer = csv.DictWriter(f, fieldnames=fields)
                    if not csv_exists:
                        writer.writeheader()
                    writer.writerow({
                        "category": cat,
                        "toy_function": name,
                        "fit_function": fitFxnName,
                        "n_toys": opts.nToys,
                        "bias_pull": bias_pull,
                        "bias_pull_err": bias_pull_err,
                        "fit_sigma": fit_sigma,
                        "fit_sigma_err": fit_sigma_err,
                        "fit_chi2": fit_chi2,
                        "fit_valid": fit_valid,
                    })
                
