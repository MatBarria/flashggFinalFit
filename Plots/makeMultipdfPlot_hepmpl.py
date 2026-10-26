#!/usr/bin/env python3
"""Plot RooMultiPdf background fits with Matplotlib (ROOT only for RooFit I/O).

Uses Plots/utils/helper.py: get_canvas(draw_ratio=True) and save_figure().
The lower panel is a DIFFERENCE panel, not a data/model ratio.
"""

import csv
import json
import math
import os
import re
from collections import OrderedDict
from optparse import OptionParser

import ROOT

# Plot without an X server (important on the LPC / batch jobs).
import matplotlib
from matplotlib.offsetbox import (
    AnchoredOffsetbox,
    DrawingArea,
    HPacker,
    VPacker,
    TextArea,
)
from matplotlib.lines import Line2D
from matplotlib.patches import Patch
from matplotlib.ticker import ScalarFormatter
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import mplhep as hep
import numpy as np

from utils.helper import get_canvas, save_figure
from utils.colors import get_color_list

# Keep the original flashggFinalFit definitions, needed for the optional
# signal workspace (years_to_process, lumiMap, sqrts__, rooiter, etc.).
from commonTools import *
from commonObjects import *


ROOT.gROOT.SetBatch(True)
ROOT.TH1.AddDirectory(False)


def get_options():
    parser = OptionParser()
    parser.add_option("--inputWSFile", dest="inputWSFile", default=None,
                      help="Input RooWorkspace containing the multipdf")
    parser.add_option("--cat", dest="cat", default="RECO_0J_PTH_0_10_Tag0",
                      help="Analysis category")
    parser.add_option("--unblind", dest="unblind", default=False,
                      action="store_true", help="Include data in signal region")
    parser.add_option("--blindingRegion", dest="blindingRegion", default="120,130",
                      help="Blinded mass interval (low,high)")
    parser.add_option("--doZeroes", dest="doZeroes", default=False,
                      action="store_true", help="Set one-event uncertainty on zero data bins")
    parser.add_option("--ext", dest="ext", default="",
                      help="Append suffix to output plots directory")
    parser.add_option("--mass", dest="mass", default=125.38, type="float",
                      help="Higgs mass")
    parser.add_option("--xvar", dest="xvar", default="CMS_hgg_mass,m_{#mu#mu},GeV",
                      help="Mass variable: name,title,units")
    parser.add_option("--nBins", dest="nBins", default=160, type="int",
                      help="Number of data bins")
    parser.add_option("--pdfNBins", dest="pdfNBins", default=3200, type="int",
                      help="Number of sampling bins for PDFs")
    parser.add_option("--translateCats", dest="translateCats", default=None,
                      help="JSON category-name translations")
    parser.add_option("--inputSignalWSFile", dest="inputSignalWSFile", default=None,
                      help="Optional signal RooWorkspace file")
    parser.add_option("--outCSV", dest="outCSV", default="out_yields.csv",
                      help="Append signal/background yield information to CSV")
    opts, _ = parser.parse_args()
    if not opts.inputWSFile:
        parser.error("--inputWSFile is required")
    if opts.nBins <= 0 or opts.pdfNBins <= 0:
        parser.error("--nBins and --pdfNBins must be positive")
    return opts


def add_background_pdf_legend(ax, pdf_names, pdf_colors, best_name):

    base_size = float(plt.rcParams["font.size"])

    family_size = 0.58 * base_size
    order_size = 0.44 * base_size

    rows = []

    for name in pdf_names:

        is_best = name == best_name
        label = label_for_pdf(name, is_best)

        order_match = re.search(
            r"\s*\(order\s+(\d+)\)", label, re.IGNORECASE
        )

        family = re.sub(
            r"\s*\(order\s+\d+\)", "", label, flags=re.IGNORECASE
        )
        family = family.replace(" (Best fit)", "")

        order_line = ""
        if order_match:
            order_line = f"Order {order_match.group(1)}"

        if is_best:
            order_line += " (Best fit)" if order_line else "Best fit"

        # Colored line
        swatch = DrawingArea(34, 16, 0, 0)
        swatch.add_artist(
            Line2D(
                [1, 31], [8, 8],
                color=pdf_colors[name],
                linewidth=2,
            )
        )

        # Function name
        label_lines = [
            TextArea(
                family,
                textprops={
                    "size": family_size,
                    "weight": "bold",
                },
            )
        ]

        # Function order
        if order_line:
            label_lines.append(
                TextArea(
                    order_line,
                    textprops={"size": order_size},
                )
            )

        label_box = VPacker(
            children=label_lines,
            align="left",
            pad=0,
            sep=0,
        )

        rows.append(
            HPacker(
                children=[swatch, label_box],
                align="center",
                pad=0,
                sep=4,
            )
        )

    pdf_box = VPacker(
        children=rows,
        align="left",
        pad=0,
        sep=9,
    )

    legend = AnchoredOffsetbox(
        loc="upper right",
        child=pdf_box,
        pad=0,
        borderpad=0,
        frameon=False,
        bbox_to_anchor=(0.98, 0.93),
        bbox_transform=ax.transAxes,
    )

    ax.add_artist(legend)


def hist_arrays(hist):
    """Convert ROOT TH1 to NumPy, including asymmetric Poisson errors."""
    nbins = hist.GetNbinsX()
    edges = np.array(
        [hist.GetXaxis().GetBinLowEdge(i) for i in range(1, nbins + 2)],
        dtype=float,
    )
    values = np.array(
        [hist.GetBinContent(i) for i in range(1, nbins + 1)], dtype=float
    )
    err_down = np.array(
        [hist.GetBinErrorLow(i) for i in range(1, nbins + 1)], dtype=float
    )
    err_up = np.array(
        [hist.GetBinErrorUp(i) for i in range(1, nbins + 1)], dtype=float
    )
    centers = (edges[:-1] + edges[1:]) / 2.0
    return centers, edges, values, np.vstack((err_down, err_up))


def label_for_pdf(name, is_best):
    """
    Convert RooFit PDF names into readable legend labels.
    Uses the same PDF labels as ROOT Version 2.
    """

    pdf_labels = {
        # DY Theory core
        "dytbern": "DY Theory core with Bernstein",
        "dytcheb": "DY Theory core with Chebychev",
        "dytpow": "DY Theory core with exp",
        "dytlau": "DY Theory core with Laurent",

        # DY Theory Bernstein core
        "dytBbern": "DY Theory Bern core with Bernstein",
        "dytBcheb": "DY Theory Bern core with Chebychev",
        "dytBpow": "DY Theory Bern core with exp",
        "dytBlau": "DY Theory Bern core with Laurent",

        # DY Theory Exponential core
        "dytEcheb": "DY Theory Exp core with Chebychev",
        "dytEpow": "DY Theory Exp core with exp",
        "dytElau": "DY Theory Exp core with Laurent",

        # DY Theory Power law core
        "dytPcheb": "DY Theory Pow core with Chebychev",
        "dytPpow": "DY Theory Pow core with exp",
        "dytPlau": "DY Theory Pow core with Laurent",

        # DY Theory FEWZ core
        "dytFcheb": "DY Theory Fewz core with Chebychev",
        "dytFpow": "DY Theory Fewz core with exp",
        "dytFlau": "DY Theory Fewz core with Laurent",

        # DY Theory FEWZ core (MC corrected)
        "dytFMcheb": "DY Theory Fewz core (MC corrected) with Chebychev",
        "dytFMpow": "DY Theory Fewz core (MC corrected) with exp",
        "dytFMlau": "DY Theory Fewz core (MC corrected) with Laurent",

        # DY Theory core (MC corrected)
        "dytMcheb": "DY Theory core (MC corrected) with Chebychev",
        "dytMpow": "DY Theory core (MC corrected) with exp",
        "dytMlau": "DY Theory core (MC corrected) with Laurent",

        # Additional background functions
        "bwzexp": "bwz x exp",
        "bwzgam": "bwzgamma",
        "2exp": "weighted sum of 3 and 2 order exp",
        "2pow": "weighted sum of 3 and 2 order pow",

        # Standard functions
        "bern": "Bernstein polynomial",
        "exp": "Sum of exponentials",
        "pow": "Sum of power law functions",
        "lau": "Laurent series",
        "mBW": "modified Breit Wigner funciton",
        "dyt": "DY Theory core with empirical function",
    }

    family = "Unknown function"
    order = None

    # Match specific names before generic names
    for pattern, label in sorted(
        pdf_labels.items(),
        key=lambda item: len(item[0]),
        reverse=True,
    ):

        match = re.search(
            re.escape(pattern),
            name,
            flags=re.IGNORECASE,
        )

        if match:
            family = label

            # Extract PDF order
            suffix = name[match.end():]

            order_match = re.fullmatch(
                r"[_-]?(\d+)",
                suffix,
            )

            if order_match:
                order = order_match.group(1)

            break

    if order is not None:
        family += f" (order {order})"

    if is_best:
        family += " (Best fit)"

    return family


def latex_axis_title(title, unit):
    if title == "m_{#mu#mu}":
        title = r"$m_{\mu\mu}$"
    elif "#" in title:
        # Basic conversion from ROOT #mu notation.
        title = "$" + title.replace("#mu", r"\mu") + "$"
    return f"{title} [{unit}]" if unit else title


def translated_category(category, jsonfile):
    if jsonfile is None:
        return category
    with open(jsonfile) as stream:
        translations = json.load(stream)
    return translations.get(category, category)


def extract_signal_histograms(opt, xvar):
    """Keep the original signal normalization and high-resolution histogram."""
    sig_file = ROOT.TFile.Open(opt.inputSignalWSFile)
    if not sig_file or sig_file.IsZombie():
        raise RuntimeError(f"Cannot open signal file: {opt.inputSignalWSFile}")
    wsig = sig_file.Get("wsig_13TeV")
    if not wsig:
        raise RuntimeError("Signal workspace wsig_13TeV not found")
    wsig.var("MH").setVal(opt.mass)

    summed = None
    # The following convention is the same as in the existing script.
    for year in years_to_process:
        norm_objects = wsig.allFunctions().selectByName(f"*{year}*normThisLumi")
        for norm in rooiter(norm_objects):
            wsig.var("IntLumi").setVal(lumiMap[year] * lumiScaleFactor)
            process = norm.GetName().split("hggpdfsmrel_")[-1].split(f"_{year}_")[0]
            object_id = f"{process}_{year}_{opt.cat}_{sqrts__}"
            norm_name = f"{outputWSObjectTitle__}_{object_id}_normThisLumi"
            pdf_name = f"{outputWSObjectTitle__}_{object_id}"
            norm_function = wsig.function(norm_name)
            signal_pdf = wsig.pdf(pdf_name)
            if not norm_function or not signal_pdf:
                raise RuntimeError(f"Missing signal PDF or normalization: {pdf_name}")

            hist = signal_pdf.createHistogram(
                f"h_pdf_{object_id}", xvar, ROOT.RooFit.Binning(opt.pdfNBins)
            )
            hist.Scale(norm_function.getVal() * (float(opt.pdfNBins) / opt.nBins))
            if summed is None:
                summed = hist.Clone("h_signal_sum")
                summed.Reset()
            summed.Add(hist)

    if summed is None:
        raise RuntimeError("No signal histograms found in wsig_13TeV")
    # Keep signal file alive until all RooFit objects have been used.
    return sig_file, summed


def calculate_yields(signal_hist, best_hist, bin_width, category):
    """Preserve the old FWHM, 121-129 GeV, and integral definitions."""
    half_max = signal_hist.GetMaximum() * 0.5
    low_bin = signal_hist.FindFirstBinAbove(half_max)
    high_bin = signal_hist.FindLastBinAbove(half_max)
    if low_bin <= 0 or high_bin <= 0:
        raise RuntimeError("Cannot determine signal FWHM")
    fwhm_low = signal_hist.GetBinCenter(low_bin)
    fwhm_high = signal_hist.GetBinCenter(high_bin)
    bkg_low_bin = best_hist.FindBin(fwhm_low)
    bkg_high_bin = best_hist.FindBin(fwhm_high)
    sig_sr_low, sig_sr_high = signal_hist.FindBin(121), signal_hist.FindBin(129)
    bkg_sr_low, bkg_sr_high = best_hist.FindBin(121), best_hist.FindBin(129)

    # ROOT histograms have *fine* bins but values in events / coarse bin.
    # Multiplying by the fine-bin width and dividing by the coarse-bin width
    # reproduces the yield convention of the original implementation.
    sig_total = signal_hist.Integral("width") / bin_width
    sig_fwhm = signal_hist.Integral(low_bin, high_bin, "width") / bin_width
    sig_sr = signal_hist.Integral(sig_sr_low, sig_sr_high, "width") / bin_width
    bkg_total = best_hist.Integral("width") / bin_width
    bkg_fwhm = best_hist.Integral(bkg_low_bin, bkg_high_bin, "width") / bin_width
    bkg_sr = best_hist.Integral(bkg_sr_low, bkg_sr_high, "width") / bin_width
    significance = sig_sr / math.sqrt(bkg_sr) if bkg_sr > 0 else float("nan")

    print(f"Signal: total={sig_total:.3f}, FWHM={sig_fwhm:.3f}, SR={sig_sr:.3f}")
    print(f"Background: total={bkg_total:.3f}, FWHM={bkg_fwhm:.3f}, SR={bkg_sr:.3f}")
    print(f"FWHM: {fwhm_high-fwhm_low:.3f} GeV,  S/sqrt(B) SR: {significance:.3f}")

    info = {
        "Category": category,
        "FWHM": round(fwhm_high - fwhm_low, 2),
        "Total Sig": round(sig_total, 2),
        "Total Bkg": round(bkg_total, 2),
        "Sig FWHM": round(sig_fwhm, 2),
        "Bkg FWHM": round(bkg_fwhm, 2),
        "Sig SR": round(sig_sr, 2),
        "Bkg SR": round(bkg_sr, 2),
        "S/sqrt(B) in SR": round(significance, 2),
    }
    return info


def append_yields_csv(csv_name, yields):
    os.makedirs(os.path.dirname(csv_name) or ".", exist_ok=True)
    needs_header = not os.path.isfile(csv_name) or os.path.getsize(csv_name) == 0
    with open(csv_name, mode="a", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=yields.keys())
        if needs_header:
            writer.writeheader()
        writer.writerow(yields)


def main():
    opt = get_options()
    cat_display = translated_category(opt.cat, opt.translateCats)
    blind_low, blind_high = [float(v) for v in opt.blindingRegion.split(",")]
    if blind_high <= blind_low:
        raise ValueError("--blindingRegion must be LOW,HIGH")

    x_name, x_title, x_units = opt.xvar.split(",", 2)
    input_file = ROOT.TFile.Open(opt.inputWSFile)
    if not input_file or input_file.IsZombie():
        raise RuntimeError(f"Cannot open: {opt.inputWSFile}")
    workspace = input_file.Get("multipdf")
    if not workspace:
        raise RuntimeError("Cannot find RooWorkspace 'multipdf'")
    xvar = workspace.var(x_name)
    if not xvar:
        raise RuntimeError(f"Cannot find variable {x_name} in workspace")
    xvar.SetTitle(x_title)
    xvar.setPlotLabel(x_title)
    xvar.setUnit(x_units)

    xmin, xmax = xvar.getMin(), xvar.getMax()
    bin_width = (xmax - xmin) / float(opt.nBins)
    xvar.setRange("SB1", 110, 120)
    xvar.setRange("SB2", 130, 150)
    print(f"[INFO] Category: {opt.cat},  x: [{xmin}, {xmax}],  "
          f"data bin width: {bin_width:g} GeV")

    multipdf_name = f"CMS_hgg_{opt.cat}_13TeV_bkgshape"
    multipdf = workspace.pdf(multipdf_name)
    if not multipdf:
        raise RuntimeError(f"Missing RooMultiPdf: {multipdf_name}")
    index_cat = workspace.cat(f"pdfindex_{opt.cat}_13TeV")
    if not index_cat:
        raise RuntimeError(f"Missing PDF index for {opt.cat}")
    best_index = index_cat.getIndex()
    if best_index < 0 or best_index >= multipdf.getNumPdfs():
        raise RuntimeError(f"Invalid best-fit PDF index: {best_index}")

    models = OrderedDict(
        (multipdf.getPdf(i).GetName(), multipdf.getPdf(i))
        for i in range(multipdf.getNumPdfs())
    )
    best_name = multipdf.getPdf(best_index).GetName()
    print(f"[INFO] Best fit PDF: {best_name} (index {best_index})")
    models[best_name].getParameters(ROOT.RooArgSet(xvar)).Print("v")

    # ROOT still does the RooFit evaluation, normalization, and histogramming.
    data = workspace.data(f"roohist_data_mass_{opt.cat}")
    if not data:
        raise RuntimeError(f"Missing data: roohist_data_mass_{opt.cat}")
    data_hist = xvar.createHistogram(
        "h_data", ROOT.RooFit.Binning(opt.nBins, xmin, xmax)
    )
    data_hist.SetBinErrorOption(ROOT.TH1.kPoisson)
    if opt.unblind:
        data.fillHistogram(data_hist, ROOT.RooArgList(xvar))
    else:
        selection = (f"{xvar.GetName()}<{blind_low:.8f}"
                     f"|{xvar.GetName()}>{blind_high:.8f}")
        reduced_data = data.reduce(selection)
        if not reduced_data:
            raise RuntimeError("Failed to select the sideband data")
        reduced_data.fillHistogram(data_hist, ROOT.RooArgList(xvar))

    data_centers, data_edges, data_values, data_errors = hist_arrays(data_hist)
    sideband_mask = ((data_centers >= 110) & (data_centers <= 120)) | (
        (data_centers >= 130) & (data_centers <= 150)
    )
    data_sb_yield = float(np.sum(data_values[sideband_mask]))
    print(f"[INFO] Data events in sidebands: {data_sb_yield:g}")

    visible = np.ones(opt.nBins, dtype=bool)
    if not opt.unblind:
        visible &= ((data_centers <= blind_low) | (data_centers >= blind_high))
    if opt.doZeroes:
        # Keep ROOT's data-error option unchanged; set the plotted errors.
        zeros = (data_values == 0) & visible
        data_errors[:, zeros] = 1.0
    # Do not draw empty bins unless --doZeroes was explicitly requested.
    plot_data = visible & ((data_values > 0) | opt.doZeroes)

    # Match the original ROOT scaling exactly: normalize each PDF to the
    # OBSERVED 110-120 plus 130-150 GeV sideband count, then turn fine-bin
    # values into predicted events / coarse (data) bin.
    pdf_hists = OrderedDict()
    pdf_arrays = OrderedDict()
    for name, model in models.items():
        hist = model.createHistogram(
            f"h_{name}", xvar, ROOT.RooFit.Binning(opt.pdfNBins, xmin, xmax)
        )
        fraction = model.createIntegral(
            ROOT.RooArgSet(xvar), ROOT.RooFit.NormSet(xvar),
            ROOT.RooFit.Range("SB1,SB2"),
        ).getVal()
        if fraction <= 0:
            raise RuntimeError(f"Invalid sideband integral for {name}: {fraction}")
        scale = data_sb_yield / fraction
        hist.Scale(scale * float(opt.pdfNBins) / opt.nBins)
        pdf_hists[name] = hist
        pdf_arrays[name] = hist_arrays(hist)
        print(f"    {name}: integral(SB1+SB2)={fraction:.6g}, scale={scale:.6g}")

    # Baseline PDF evaluated in the same bins as the data (for subtraction).
    best_fraction = models[best_name].createIntegral(
        ROOT.RooArgSet(xvar), ROOT.RooFit.NormSet(xvar),
        ROOT.RooFit.Range("SB1,SB2"),
    ).getVal()
    best_coarse = models[best_name].createHistogram(
        "h_best_fit_data_binning", xvar,
        ROOT.RooFit.Binning(opt.nBins, xmin, xmax),
    )
    best_coarse.Scale(data_sb_yield / best_fraction)
    _, _, best_coarse_values, _ = hist_arrays(best_coarse)
    data_residual = data_values - best_coarse_values

    # Optional signal: use original flashggFinalFit workspaces and yields.
    signal_hist = None
    yields = None
    if opt.inputSignalWSFile:
        signal_file, signal_hist = extract_signal_histograms(opt, xvar)
        yields = calculate_yields(signal_hist, pdf_hists[best_name], bin_width,
                                  cat_display)

    # Match the style used throughout Plots/: all font sizes and the default
    # figure dimensions come from hep.style.CMS and utils.helper.get_canvas().
    # Do not override rcParams, figsize, fontsize, or the helper's spacing.
    plt.style.use(hep.style.CMS)
    fig, axs = get_canvas(True)
    ax_top, ax_bottom = axs

    # Use the same background palette helper as the other plotting scripts.
    palette = get_color_list(len(models))
    model_colors = {name: palette[i] for i, name in enumerate(models)}

    best_x, best_edges, best_y, _ = pdf_arrays[best_name]
    for name, (xs, edges, ys, _) in pdf_arrays.items():
        is_best = name == best_name
        color = model_colors[name]

        # PDFs are represented by fine-binned histograms (default 3200 bins),
        # so hep.histplot produces visually smooth fit curves. Same convention
        # and default font sizes as in the supplied examples.
        hep.histplot(
            ys,
            edges,
            histtype="step",
            yerr=False,
            label=label_for_pdf(name, is_best),
            color=color,
            linewidth=2,
            ax=ax_top,
        )

        # IMPORTANT: the lower panel is a subtraction, NOT a ratio.
        hep.histplot(
            ys - best_y,
            edges,
            histtype="step",
            yerr=False,
            color=color,
            linewidth=2,
            ax=ax_bottom,
        )

    # Keep the original signal overlay confined to the blind interval.
    if signal_hist is not None:
        sig_x, _, sig_y, _ = hist_arrays(signal_hist)
        in_signal_window = (sig_x >= blind_low) & (sig_x <= blind_high)
        ax_top.fill_between(
            sig_x, 0, sig_y, where=in_signal_window,
            color="blue", alpha=0.3, interpolate=True,
        )
        ax_top.plot(
            sig_x[in_signal_window], sig_y[in_signal_window],
            color="blue", linewidth=2, label="Signal",
        )
        ax_bottom.fill_between(
            sig_x, 0, sig_y, where=in_signal_window,
            color="blue", alpha=0.3, interpolate=True,
        )
        ax_bottom.plot(
            sig_x[in_signal_window], sig_y[in_signal_window],
            color="blue", linewidth=2,
        )

    # Use ax.errorbar, like the BDT-category example, because we need to
    # exclude blinded data bins (a full histplot would show them as zeros).
    data_legend_handle = ax_top.errorbar(
        data_centers[plot_data],
        data_values[plot_data],
        yerr=data_errors[:, plot_data],
        fmt="o",
        color="black",
        markersize=5,
        label="Data" if opt.unblind else "Data",
        zorder=5,
    )
    ax_bottom.errorbar(
        data_centers[plot_data],
        data_residual[plot_data],
        yerr=data_errors[:, plot_data],
        fmt="o",
        color="black",
        markersize=5,
        zorder=5,
    )

    for ax in axs:
        ax.set_xlim(xmin, xmax)
        for border in (120, 130):
            if xmin < border < xmax:
                ax.axvline(
                    x=border,
                    color="grey",
                    linestyle="--",
                    alpha=0.5,
                )

    # CMS labeling, axis fonts and legend fonts are NOT explicitly resized.
    # This follows draw_data_and_simul_and_ratio and find_bdt_categories.
    # hep.cms.label(data=True, label="", com="13.6", ax=ax_top)
    hep.cms.label(
        data=True,
        label="",
        com="13.6",
        lumi="307.54",
        ax=ax_top,
    )
    ax_top.set_ylabel(f"Events / {bin_width:g} GeV", loc="center")
    max_top = max(
        np.max(data_values[visible] + data_errors[1, visible])
        if np.any(visible) else 0,
        *(float(np.max(p[2])) for p in pdf_arrays.values()),
        float(np.max(sig_y[in_signal_window]))
        if signal_hist is not None and np.any(in_signal_window) else 0,
    )
    ax_top.set_ylim(0, max(1.0, 1.3 * max_top))
    ax_top.tick_params(
        axis="x", which="both", bottom=True, top=True, labelbottom=False
    )

    ax_top.ticklabel_format(axis="y", style="sci", scilimits=(0, 0))

# Use ×10³ instead of 1e3
    formatter = ScalarFormatter(useMathText=True)
    formatter.set_scientific(True)
    formatter.set_powerlimits((0, 0))
    ax_top.yaxis.set_major_formatter(formatter)

# Move scientific notation to the left of CMS
    offset_text = ax_top.yaxis.get_offset_text()
    offset_text.set_x(-0.03)
    offset_text.set_ha("right")

    # Match the original top-panel content; use CMS's default font size.
    # if yields is None:
    plot_annotation = cat_display
    # else:
        # plot_annotation = (
            # f"{cat_display}\n"
            # rf"$S/\sqrt{{B}}$ in SR: {yields['S/sqrt(B) in SR']:.2f}"
            # "\n"
            # f"S = {yields['Total Sig']:.2f}  "
            # f"B = {yields['Total Bkg']:.2f}  "
            # f"FWHM = {yields['FWHM']:.2f} GeV"
        # )
    # ax_top.text(
        # 0.02, 0.94, plot_annotation, transform=ax_top.transAxes, va="top"
    # )

    ax_top.text(0.05, 0.94, plot_annotation, transform=ax_top.transAxes,
            fontsize=21, fontweight="bold", ha="left", va="top")
    # ax_top.legend(
        # frameon=False,
        # loc="upper right",
        # ncols=2 if len(models) > 4 else 1,
    # )

# -------------------------------------------
# Data and signal legend
# -------------------------------------------

    data_signal_handles = [data_legend_handle]
    data_signal_labels = ["Data"]

    if signal_hist is not None:

        signal_handle = Patch(
            facecolor="royalblue",
            edgecolor="royalblue",
            alpha=0.35,
        )

        data_signal_handles.append(signal_handle)
        data_signal_labels.append("S model")


    ax_top.legend(
        data_signal_handles,
        data_signal_labels,
        frameon=False,
        loc="upper left",
        bbox_to_anchor=(0.05, 0.82),
        ncol=2,
        handlelength=1.2,
        columnspacing=1.5,
        borderaxespad=0,
        fontsize=0.62 * float(plt.rcParams["font.size"]),
    )


# -------------------------------------------
# Background PDF legend
# -------------------------------------------

    add_background_pdf_legend(
        ax_top,
        models.keys(),
        model_colors,
        best_name,
    )



    ax_bottom.axhline(y=0, color="grey", linestyle="--", alpha=0.5)
    ax_bottom.set_ylabel("Events - best fit", loc="center")
    ax_bottom.set_xlabel(latex_axis_title(x_title, x_units))

    # Autoscale using the displayed data and the alternative PDF curves.
    residual_limit = max(
        float(np.max(np.abs(data_residual[visible]) +
                     np.max(data_errors[:, visible], axis=0)))
        if np.any(visible) else 0.0,
        *(float(np.max(np.abs(p[2] - best_y))) for p in pdf_arrays.values()),
        float(np.max(np.abs(sig_y[in_signal_window])))
        if signal_hist is not None and np.any(in_signal_window) else 0.0,
    )
    residual_limit = max(1.0, 1.05 * residual_limit)
    ax_bottom.set_ylim(-residual_limit, residual_limit)

    # helper.save_figure concatenates strings, so directory MUST end with '/'.
    ext_string = f"_{opt.ext}" if opt.ext else ""
    # helper.save_figure concatenates strings, so directory MUST end with '/'.
    directory = f"plots{ext_string}/mplhep/"
    save_figure(fig, directory, f"multipdf_plot_{opt.cat}")
    plt.close(fig)

    if yields is not None:
        append_yields_csv(directory + opt.outCSV, yields)
        print(f"[INFO] Updated {opt.outCSV}")

    input_file.Close()
    if opt.inputSignalWSFile:
        signal_file.Close()


if __name__ == "__main__":
    main()
