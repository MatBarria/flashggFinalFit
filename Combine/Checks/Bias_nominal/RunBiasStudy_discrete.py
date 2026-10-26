#!/usr/bin/env python3
"""Discrete-profiled bias study: alternative toy RooMultiPdf -> nominal fit RooMultiPdf.

One invocation processes one generating PDF index and one batch of N toys.
The fitting PDF index is NEVER frozen: Combine profiles over the nominal PDFs.
"""
import argparse
import csv
import glob
import math
import os
from pathlib import Path
import shlex
import shutil
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('-d', '--toyDatacard', required=True, help='ROOT workspace used to GENERATE pseudo-data')
parser.add_argument('-w', '--fitDatacard', required=True, help='ROOT workspace used to FIT pseudo-data')
parser.add_argument('--toyWorkspace', default='w')
parser.add_argument('--fitWorkspace', default='w')
parser.add_argument('-b', '--BDTCategory', required=True)
parser.add_argument('--toyIndex', type=int, default=0)
parser.add_argument('-n', '--nToys', type=int, default=200)
parser.add_argument('-e', '--expectSignal', type=float, default=1.)
parser.add_argument('-m', '--mH', type=float, default=125.)
parser.add_argument('--poi', default='r')
parser.add_argument('-s', '--seed', type=int, default=-1)
parser.add_argument('--listFunctions', action='store_true', help='Inspect both workspaces and exit')
parser.add_argument('--dryRun', action='store_true')
parser.add_argument('--toysNoSystematics', action='store_true', help='Use this only when intentionally doing a no-nuisance-fluctuation study')
parser.add_argument('--statOnly', action='store_true', help='Freeze CMS_hmm_peak/sigma in toy and fit workspaces; leave fit PDF index profiled')
parser.add_argument('--outCSV', default=None)
parser.add_argument('--outPullCSV', default=None, help='One CSV row per accepted toy for later split merging')
parser.add_argument('-c', '--combineOptions', default=(
    '--setParameterRanges r=-20,20 '
    '--cminDefaultMinimizerStrategy 1 '
    '--X-rtd MINIMIZER_freezeDisassociatedParams '
    '--X-rtd MINIMIZER_multiMin_hideConstants '
    '--X-rtd MINIMIZER_multiMin_maskConstraints'
))
args = parser.parse_args()
if args.nToys <= 0 or args.toyIndex < 0:
    parser.error('--nToys must be positive and --toyIndex >= 0')
if args.toyDatacard == args.fitDatacard:
    parser.error('Use different workspace paths for toy generation and nominal fitting')

import ROOT as r

library_dir = os.environ.get('BIAS_LIBRARY_DIR')
if not library_dir:
    base, arch = os.environ.get('CMSSW_BASE'), os.environ.get('SCRAM_ARCH')
    if base and arch:
        library_dir = os.path.join(base, 'lib', arch)
if not library_dir:
    raise RuntimeError('Initialize CMSSW (cmsenv) or set BIAS_LIBRARY_DIR')
for lib in ('libHiggsAnalysisCombinedLimit.so', 'libflashggFinalFitDYCore.so'):
    library = os.path.join(library_dir, lib)
    if r.gSystem.Load(library) < 0:
        raise RuntimeError(f'Cannot load {library}')
r.gROOT.SetBatch(True)


def argset_list(argset):
    iterator = argset.createIterator()
    found = []
    while True:
        item = iterator.Next()
        if not item:
            break
        found.append(item)
    return found


def load_multipdf(path, workspace_name):
    tf = r.TFile.Open(path)
    if not tf or tf.IsZombie():
        raise RuntimeError(f'Cannot open ROOT workspace: {path}')
    ws = tf.Get(workspace_name)
    if not ws:
        raise RuntimeError(f'Workspace {workspace_name!r} missing in {path}')
    multipdfs = [pdf for pdf in argset_list(ws.allPdfs()) if pdf.InheritsFrom('RooMultiPdf')]
    indices = [cat for cat in argset_list(ws.allCats()) if cat.GetName().startswith('pdfindex')]
    if len(multipdfs) != 1 or len(indices) != 1:
        raise RuntimeError(f'{path}: expected exactly one RooMultiPdf and one pdfindex; '
                           f'found {len(multipdfs)} and {len(indices)}')
    mp = multipdfs[0]
    index = indices[0].GetName()
    names = [mp.getPdf(i).GetName() for i in range(mp.getNumPdfs())]
    return tf, ws, mp, index, names


toy_file, toy_ws, toy_mp, toy_index_name, toy_names = load_multipdf(args.toyDatacard, args.toyWorkspace)
fit_file, fit_ws, fit_mp, fit_index_name, fit_names = load_multipdf(args.fitDatacard, args.fitWorkspace)
print(f'TOY: {toy_mp.GetName()}, pdfindex={toy_index_name}, {len(toy_names)} functions')
for i, name in enumerate(toy_names):
    print(f'    toy index {i}: {name}')
print(f'FIT: {fit_mp.GetName()}, pdfindex={fit_index_name}, {len(fit_names)} functions')
for i, name in enumerate(fit_names):
    print(f'    fit index {i}: {name}')
if args.listFunctions:
    raise SystemExit(0)
if args.toyIndex >= len(toy_names):
    raise RuntimeError(f'--toyIndex {args.toyIndex} outside 0..{len(toy_names)-1}')


def signal_shape_vars(ws, label):
    """Find existing signal-shape nuisance parameters for this category."""

    expected_suffix = '_' + args.BDTCategory

    names = sorted(
        v.GetName() for v in argset_list(ws.allVars())
        if v.GetName().endswith(expected_suffix)
        and (
            v.GetName().startswith('CMS_hmm_peak_')
            or v.GetName().startswith('CMS_hmm_sigma_')
        )
    )

    print(
        f'{label}: signal-shape parameters to freeze: {names}',
        flush=True,
    )

    return names


toy_signal_vars = signal_shape_vars(toy_ws, 'TOY') if args.statOnly else []
fit_signal_vars = signal_shape_vars(fit_ws, 'FIT') if args.statOnly else []
if args.statOnly:
    print('STAT-ONLY toy set/freeze:', toy_signal_vars, flush=True)
    print('STAT-ONLY fit set/freeze:', fit_signal_vars, flush=True)


def call(cmd):
    print('RUN:', shlex.join(cmd), flush=True)
    if not args.dryRun:
        subprocess.run(cmd, check=True)


def move_combine_output(label, method, destination):
    if args.dryRun:
        return
    candidates = glob.glob(f'higgsCombine_{label}.{method}.mH*.root')
    if len(candidates) != 1:
        raise RuntimeError(f'Expected exactly one Combine {method} output; found {candidates}')
    shutil.move(candidates[0], destination)
    if not os.path.isfile(destination) or os.path.getsize(destination) == 0:
        raise RuntimeError(f'Missing/empty Combine output: {destination}')


label = f'{args.BDTCategory}_toyPDF{args.toyIndex}'
for directory in ('BiasToys', 'BiasFits', 'BiasPlots'):
    Path(directory).mkdir(exist_ok=True)
toy_output = f'BiasToys/{label}.root'
fit_output = f'BiasFits/{label}_discrete.root'
for output in (toy_output, fit_output):
    if Path(output).exists() and not args.dryRun:
        raise RuntimeError(f'Refusing to overwrite an existing result: {output}')

toy_cmd = [
    'combine', '-m', str(args.mH), '-d', args.toyDatacard,
    '-M', 'GenerateOnly', '--expectSignal', str(args.expectSignal),
    '-s', str(args.seed), '--saveToys', '-t', str(args.nToys),
    '-n', f'_{label}',
    '--setParameters', ','.join([
        f'{toy_index_name}={args.toyIndex}',
        f'{args.poi}={args.expectSignal}',
        *(f'{name}=0' for name in toy_signal_vars),
    ]),
    '--freezeParameters', ','.join([toy_index_name, 'MH', *toy_signal_vars]),
]
if args.toysNoSystematics:
    toy_cmd.append('--toysNoSystematics')
call(toy_cmd)
move_combine_output(label, 'GenerateOnly', toy_output)

fit_cmd = [
    'combine', '-m', str(args.mH), '-d', args.fitDatacard,
    '-M', 'MultiDimFit', '-P', args.poi, '--algo', 'singles',
    '--robustFit', '1',
    '--cminFallbackAlgo', 'Minuit2,Migrad,1:0.1',
    '--cminFallbackAlgo', 'Minuit2,Simplex,1:0.5',
    '--saveSpecifiedIndex', fit_index_name,
    *shlex.split(args.combineOptions),
    '-t', str(args.nToys), '-n', f'_{label}_discrete',
    '--toysFile', toy_output,
    '--setParameters', ','.join([
        f'{args.poi}={args.expectSignal}',
        *(f'{name}=0' for name in fit_signal_vars),
    ]),
    '--freezeParameters', ','.join(['MH', *fit_signal_vars]),
]
if args.toysNoSystematics:
    fit_cmd.append('--toysNoSystematics')
# CRITICAL: Do not add `--freezeParameters <fit_index_name>` here.
call(fit_cmd)
move_combine_output(f'{label}_discrete', 'MultiDimFit', fit_output)

if args.dryRun:
    raise SystemExit(0)

# Aggregate by iToy rather than relying on the order of the three quantiles.
tf = r.TFile.Open(fit_output)
if not tf or tf.IsZombie():
    raise RuntimeError(f'Cannot open discrete fit output: {fit_output}')
tree = tf.Get('limit')
if not tree:
    raise RuntimeError(f'Missing limit tree in {fit_output}')
if not tree.GetBranch(fit_index_name):
    raise RuntimeError(f'--saveSpecifiedIndex failed: no {fit_index_name} branch in {fit_output}')
groups = {}
for entry in range(tree.GetEntries()):
    tree.GetEntry(entry)
    itoy = int(getattr(tree, 'iToy'))
    q = float(getattr(tree, 'quantileExpected'))
    value = float(getattr(tree, args.poi))
    by_toy = groups.setdefault(itoy, {})
    if abs(q + 1.) < 1.e-4:
        key = 'best'
        by_toy['fit_index'] = int(getattr(tree, fit_index_name))
    elif abs(q + 0.32) < 0.005:
        key = 'low'
    elif abs(q - 0.32) < 0.005:
        key = 'high'
    else:
        continue
    if key in by_toy:
        raise RuntimeError(f'Duplicate {key} entry for toy {itoy} in {fit_output}')
    by_toy[key] = value

# Record selected best-fit indices independently of the confidence interval quality.
best_counts = {i: 0 for i in range(len(fit_names))}
for toy in groups.values():
    ix = toy.get('fit_index')
    if ix is not None:
        if ix not in best_counts:
            raise RuntimeError(f'Unexpected profiled fit index {ix} in {fit_output}')
        best_counts[ix] += 1

# Pulls reject incomplete/nonfinite/nonordered and POI boundary-clipped intervals.
poi_range = None
tokens = shlex.split(args.combineOptions)
for pos, tok in enumerate(tokens):
    spec = tokens[pos + 1] if tok == '--setParameterRanges' and pos + 1 < len(tokens) else (
        tok.split('=', 1)[1] if tok.startswith('--setParameterRanges=') else None
    )
    if spec is None:
        continue
    for assignment in spec.split(':'):
        name, equal, limits = assignment.partition('=')
        if equal and name == args.poi:
            poi_range = tuple(map(float, limits.split(',', 1)))
            break
boundary_tol = max(1.e-6, 1.e-5 * (poi_range[1] - poi_range[0])) if poi_range else 0.
pulls = []
pull_records = []
for itoy, toy in groups.items():
    if not all(k in toy for k in ('best', 'low', 'high')):
        continue
    best, lo, hi = toy['best'], toy['low'], toy['high']
    if not all(math.isfinite(x) for x in (best, lo, hi)) or not lo < best < hi:
        continue
    if poi_range and (lo <= poi_range[0] + boundary_tol or hi >= poi_range[1] - boundary_tol):
        continue
    pull = (best - args.expectSignal) / (0.5 * (hi - lo))
    pulls.append(pull)
    pull_records.append({
        'toy_id': itoy, 'pull': pull, 'r_best': best, 'r_low': lo, 'r_high': hi,
        'fit_index': toy.get('fit_index', ''),
    })
if len(groups) > args.nToys:
    raise RuntimeError(f'Found {len(groups)} toy IDs for only {args.nToys} toys')
failed = args.nToys - len(pulls)
print(f'Toy PDF {args.toyIndex}: {len(pulls)}/{args.nToys} valid pulls, {failed} invalid/missing')
print('Best-fit PDF selection counts:', best_counts)
# Keep per-toy pulls so the aggregate Gaussian is fitted only once across ALL splits.
outpull = args.outPullCSV or f'bias_pulls_{args.BDTCategory}.csv'
if Path(outpull).exists():
    raise RuntimeError(f'Refusing to overwrite existing pull CSV: {outpull}')
with open(outpull, 'w', newline='') as stream:
    writer = csv.DictWriter(stream, fieldnames=(
        'toy_id', 'pull', 'r_best', 'r_low', 'r_high', 'fit_index',
    ))
    writer.writeheader()
    writer.writerows(sorted(pull_records, key=lambda row: row['toy_id']))
print(f'Wrote {outpull} containing {len(pull_records)} valid pulls')

hist = r.TH1F(f'pull_{label}', f'{args.BDTCategory} discrete profiling;Pull;Toys', 160, -8, 8)
for pull in pulls:
    hist.Fill(pull)
canvas = r.TCanvas(f'c_{label}', '', 800, 600)
hist.Draw()
valid = 0
mu = mu_err = sigma = sigma_err = chi2 = float('nan')
if len(pulls) >= 5:
    result = hist.Fit('gaus', 'SQ0')
    fit = hist.GetFunction('gaus')
    if fit and int(result) == 0:
        mu, mu_err = fit.GetParameter(1), fit.GetParError(1)
        sigma, sigma_err, chi2 = fit.GetParameter(2), fit.GetParError(2), fit.GetChisquare()
        valid = 1
for ext in ('pdf', 'png'):
    canvas.SaveAs(f'BiasPlots/{label}_discrete_pulls.{ext}')
canvas.Close()

# Preserve both full names and counts; the summary plot groups on toy_function.
toy_name = toy_names[args.toyIndex]
short_toy = toy_name.split('_')[-1]
short_names = [name.split('_')[-1] for name in toy_names]
if len(set(short_names)) == len(short_names):
    toy_name = short_toy
fieldnames = [
    'category', 'toy_function', 'toy_index', 'fit_function', 'fit_method',
    'n_toys', 'bias_pull', 'bias_pull_err', 'fit_sigma', 'fit_sigma_err',
    'fit_chi2', 'fit_valid', 'n_valid', 'n_failed',
    'bestfit_index_counts', 'bestfit_function_counts',
]
best_function_counts = {fit_names[i]: value for i, value in best_counts.items()}
outcsv = args.outCSV or f'bias_results_{args.BDTCategory}.csv'
if Path(outcsv).exists():
    raise RuntimeError(f'Refusing to overwrite existing CSV: {outcsv}')
with open(outcsv, 'w', newline='') as stream:
    writer = csv.DictWriter(stream, fieldnames=fieldnames)
    writer.writeheader()
    writer.writerow({
        'category': args.BDTCategory, 'toy_function': toy_name, 'toy_index': args.toyIndex,
        'fit_function': 'discrete', 'fit_method': 'discrete',
        'n_toys': args.nToys, 'bias_pull': mu, 'bias_pull_err': mu_err,
        'fit_sigma': sigma, 'fit_sigma_err': sigma_err, 'fit_chi2': chi2,
        'fit_valid': valid, 'n_valid': len(pulls), 'n_failed': failed,
        'bestfit_index_counts': str(best_counts),
        'bestfit_function_counts': str(best_function_counts),
    })
tf.Close()
toy_file.Close()
fit_file.Close()
print(f'Wrote {outcsv}; fit_valid={valid}')
