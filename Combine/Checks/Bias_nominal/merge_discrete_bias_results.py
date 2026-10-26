#!/usr/bin/env python3
"""Combine per-job pulls into ONE Gaussian fit per category and generating PDF.

Run from Combine/Checks/Bias_nominal with an initialized CMSSW ROOT environment,
after every job from create_discrete_bias_jobs.py has successfully completed.
"""
import argparse
import ast
import csv
from collections import defaultdict
import math
from pathlib import Path
from statistics import median

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--input-dir', type=Path, default=Path('condor_discrete'))
parser.add_argument('--output-csv', type=Path, default=Path('bias_results_merged.csv'))
parser.add_argument('--pulls-csv', type=Path, default=Path('bias_pulls_merged.csv'))
parser.add_argument('--plot-dir', type=Path, default=Path('BiasPlots_merged'))
parser.add_argument('--validate-only', action='store_true',
                    help='Check all job outputs without importing ROOT or writing files')
parser.add_argument('--skip-category', nargs='+', default=[], help='Categories to exclude from merging')
args = parser.parse_args()

manifest_path = args.input_dir / 'manifest.csv'
if not manifest_path.is_file():
    parser.error(f'Missing manifest: {manifest_path}')
with manifest_path.open(newline='') as stream:
    manifest = list(csv.DictReader(stream))
    manifest = [
        m for m in manifest
        if m['category'] not in args.skip_category
    ]
if not manifest:
    parser.error(f'No jobs listed in {manifest_path}')

expected_keys = set()
groups = defaultdict(lambda: {
    'pulls': [], 'records': [], 'n_toys': 0, 'n_valid': 0,
    'best_index_counts': defaultdict(int),
    'best_function_counts': defaultdict(int),
    'name': None, 'fit_function': None, 'fit_method': None,
    'splits': set(),
})
problems = []
for m in manifest:
    try:
        category = m['category']
        pdf_index = int(m['toy_index'])
        split = int(m['split'])
        batch_toys = int(m['n_toys'])
        jobdir = args.input_dir / m['job_dir']
    except (KeyError, ValueError) as exc:
        problems.append(f'Invalid manifest row {m}: {exc}')
        continue
    key = (category, pdf_index, split)
    if key in expected_keys:
        problems.append(f'Duplicate manifest job: {key}')
        continue
    expected_keys.add(key)
    if batch_toys < 1:
        problems.append(f'Invalid n_toys in {key}: {batch_toys}')
        continue
    resultdir = jobdir / 'BiasResults'
    summary_path = resultdir / f'bias_results_{category}.csv'
    pulls_path = resultdir / f'bias_pulls_{category}.csv'
    if not summary_path.is_file() or not pulls_path.is_file():
        problems.append(f'Missing outputs for {key}: {resultdir}')
        continue
    try:
        with summary_path.open(newline='') as stream:
            summaries = list(csv.DictReader(stream))
        with pulls_path.open(newline='') as stream:
            pull_rows = list(csv.DictReader(stream))
        if len(summaries) != 1:
            raise ValueError(f'Expected exactly one summary row; found {len(summaries)}')
        s = summaries[0]
        if (s['category'], int(s['toy_index'])) != (category, pdf_index):
            raise ValueError('Summary category/PDF does not match manifest')
        if int(s['n_toys']) != batch_toys:
            raise ValueError('Summary toy count does not match manifest')
        if int(s['n_valid']) != len(pull_rows):
            raise ValueError('Summary n_valid differs from pull CSV rows')
        if int(s['n_failed']) != batch_toys - len(pull_rows):
            raise ValueError('Summary n_failed differs from total minus valid')
        index_counts = ast.literal_eval(s['bestfit_index_counts'])
        function_counts = ast.literal_eval(s['bestfit_function_counts'])
        if not isinstance(index_counts, dict) or not isinstance(function_counts, dict):
            raise ValueError('Malformed PDF selection counts')
        unique_toys = set()
        values = []
        records = []
        for r in pull_rows:
            toy_id = int(r['toy_id'])
            if toy_id in unique_toys:
                raise ValueError(f'Duplicate toy_id={toy_id} within split')
            unique_toys.add(toy_id)
            pull = float(r['pull'])
            if not math.isfinite(pull):
                raise ValueError(f'Nonfinite pull for toy_id={toy_id}')
            values.append(pull)
            records.append({
                'category': category, 'toy_index': pdf_index,
                'split': split, 'toy_id': toy_id,
                'pull': pull,
                'r_best': r['r_best'], 'r_low': r['r_low'],
                'r_high': r['r_high'], 'fit_index': r['fit_index'],
            })
        group = groups[(category, pdf_index)]
        if group['name'] is None:
            group['name'] = s['toy_function']
            group['fit_function'] = s['fit_function']
            group['fit_method'] = s['fit_method']
        if (s['toy_function'], s['fit_function'], s['fit_method']) != (
            group['name'], group['fit_function'], group['fit_method']
        ):
            raise ValueError('Mismatch in PDF/function names across splits')
        group['pulls'].extend(values)
        group['records'].extend(records)
        group['n_toys'] += batch_toys
        group['n_valid'] += len(values)
        group['splits'].add(split)
        for i, count in index_counts.items():
            group['best_index_counts'][int(i)] += int(count)
        for name, count in function_counts.items():
            group['best_function_counts'][str(name)] += int(count)
    except (OSError, ValueError, KeyError, SyntaxError, TypeError) as exc:
        problems.append(f'Invalid result for {key}: {exc}')

if problems:
    print(f'Cannot merge; {len(problems)} problems:')
    for issue in problems:
        print(' -', issue)
    raise SystemExit(1)

print(f'Validated {len(expected_keys)} split jobs covering {len(groups)} generating PDFs.')
for (cat, pdf), group in sorted(groups.items()):
    print(f'  {cat} toyPDF{pdf}: {group["n_valid"]}/{group["n_toys"]} valid pulls from {len(group["splits"])} splits')
if args.validate_only:
    raise SystemExit(0)

try:
    import ROOT as r
except ImportError as exc:
    parser.error('PyROOT is required for Gaussian fits; run `cmsenv` before merging')
r.gROOT.SetBatch(True)
r.gStyle.SetOptStat(1110)
r.gStyle.SetOptFit(1111)

for path in (args.output_csv, args.pulls_csv):
    if path.exists():
        parser.error(f'Refusing to overwrite existing merged file: {path}')
# Keep histogram-only and Gaussian-fit plots in separate directories.
plots_without_fit = args.plot_dir / 'without_gaussian_fit'
plots_with_fit = args.plot_dir / 'with_gaussian_fit'
plots_without_fit.mkdir(parents=True, exist_ok=True)
plots_with_fit.mkdir(parents=True, exist_ok=True)

fieldnames = [
    'category', 'toy_function', 'toy_index', 'fit_function', 'fit_method',
    'n_toys',
    'bias_pull',
    'bias_pull_median',
    'bias_pull_err',
    'fit_sigma', 'fit_sigma_err',
    'fit_chi2', 'fit_valid', 'n_valid', 'n_failed',
    'bestfit_index_counts', 'bestfit_function_counts', 'n_splits',
]
pull_fieldnames = [
    'category', 'toy_index', 'split', 'toy_id', 'pull',
    'r_best', 'r_low', 'r_high', 'fit_index',
]
with args.output_csv.open('w', newline='') as summary_stream, \
     args.pulls_csv.open('w', newline='') as pulls_stream:
    writer = csv.DictWriter(summary_stream, fieldnames=fieldnames)
    writer.writeheader()
    raw_writer = csv.DictWriter(pulls_stream, fieldnames=pull_fieldnames)
    raw_writer.writeheader()
    for (category, pdf_index), group in sorted(groups.items()):
        raw_writer.writerows(sorted(group['records'], key=lambda x: (x['split'], x['toy_id'])))
        # Same pull histogram and Gaussian fit as RunBiasStudy_discrete.py.
        ident = f'{category}_toyPDF{pdf_index}'
        # Use the generating toy PDF name from the per-job summary CSV.
        plot_title = f'{category} | Toy PDF: {group["name"]} (index {pdf_index});Pull;Toys'
        hist = r.TH1F(f'pull_{ident}_merged', plot_title, 160, -8, 8)
        hist.SetDirectory(0)
        for pull in group['pulls']:
            hist.Fill(pull)

        # Clone BEFORE the fit so the no-fit plot has no attached Gaussian.
        hist_no_fit = hist.Clone(f'pull_{ident}_no_fit')
        hist_no_fit.SetDirectory(0)

        # Median of all valid pulls, combined across the completed splits.
        pull_median = median(group['pulls']) if group['pulls'] else float('nan')
        mu = mu_err = sigma = sigma_err = chi2 = float('nan')
        fit_valid = 0
        fit = None
        if len(group['pulls']) >= 5:
            # '0' suppresses automatic drawing; draw manually in the fit plot.
            # Fit calculation and histogram are unchanged from the original.
            result = hist.Fit('gaus', 'SQ0')
            candidate = hist.GetFunction('gaus')
            if candidate and int(result) == 0:
                fit = candidate
                fit.SetLineColor(r.kRed)
                fit.SetLineWidth(3)
                mu = fit.GetParameter(1)
                mu_err = fit.GetParError(1)
                sigma = fit.GetParameter(2)
                sigma_err = fit.GetParError(2)
                chi2 = fit.GetChisquare()
                fit_valid = 1

        # Save one plain histogram and one histogram with Gaussian-fit curve.
        for variant, plot_hist, plot_dir in (
            ('without_fit', hist_no_fit, plots_without_fit),
            ('with_fit', hist, plots_with_fit),
        ):
            canvas = r.TCanvas(f'c_{ident}_{variant}', '', 800, 600)
            plot_hist.Draw('hist')

            # Overlay only a successful Gaussian fit on the with-fit version.
            if variant == 'with_fit' and fit_valid:
                fit.Draw('same')
            elif variant == 'with_fit':
                notice = r.TLatex()
                notice.SetNDC(True)
                notice.SetTextSize(0.035)
                notice.DrawLatex(0.13, 0.83, 'Gaussian fit unavailable')

            canvas.Update()
            # Position the ROOT histogram/fit statistics box.
            stats = plot_hist.GetListOfFunctions().FindObject('stats')
            if stats:
                stats.SetX1NDC(0.61)
                stats.SetX2NDC(0.90)
                stats.SetY1NDC(0.59 if variant == 'with_fit' else 0.72)
                stats.SetY2NDC(0.90)
                stats.SetTextSize(0.028)
                canvas.Modified()
                canvas.Update()

            for ext in ('pdf', 'png'):
                canvas.SaveAs(str(plot_dir / f'{ident}_discrete_pulls.{ext}'))
            canvas.Close()

        writer.writerow({
            'category': category, 'toy_function': group['name'],
            'toy_index': pdf_index, 'fit_function': group['fit_function'],
            'fit_method': group['fit_method'], 'n_toys': group['n_toys'],
            'bias_pull': f'{mu:.4f}',
            'bias_pull_median': f'{pull_median:.4f}',
            'bias_pull_err': f'{mu_err:.4f}',
            'fit_sigma': f'{sigma:.4f}',
            'fit_sigma_err': f'{sigma_err:.4f}',
            'fit_chi2': f'{chi2:.4f}',
            'fit_valid': fit_valid,
            'n_valid': group['n_valid'],
            'n_failed': group['n_toys'] - group['n_valid'],
            'bestfit_index_counts': str(dict(sorted(group['best_index_counts'].items()))),
            'bestfit_function_counts': str(dict(sorted(group['best_function_counts'].items()))),
            'n_splits': len(group['splits']),
        })
print(f'Wrote {args.output_csv}, {args.pulls_csv}, and plots to {plots_without_fit}/ and {plots_with_fit}/')
