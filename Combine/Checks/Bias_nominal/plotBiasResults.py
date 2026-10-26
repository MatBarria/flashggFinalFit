import numpy as np
import matplotlib.pyplot as plt
import mplhep as hep

from optparse import OptionParser
parser = OptionParser()
parser.add_option("-c","--inputCSV",default="bias_results.root")
(opts,args) = parser.parse_args()

plt.style.use(hep.style.CMS)

data = np.genfromtxt(
    opts.inputCSV,
    delimiter=",",
    names=True,
    dtype=None,
    encoding="utf-8"
)

def plotBiasSummary(cat_data, outputFolder):
    bkg_funcs = np.unique(cat_data["toy_function"])
    fit_funcs = np.unique(cat_data["fit_function"])
    category = np.unique(cat_data["category"])

    if len(category) > 1:
        raise ValueError("Can only plot one category at a time")

    if np.array_equal(np.sort(bkg_funcs), np.sort(fit_funcs)) != True:
        print("[WARNING]: not all functions have fitted properly")
        print("Category: " + category[0] + "\nFit Functions: " + str(fit_funcs) + "\nToy Functions: " + str(bkg_funcs))
        #raise ValueError("not all functions have fitted properly")


    fig, ax = plt.subplots()

    # acceptable bias band
    ax.axhspan(-0.2, 0.2, color="gray", alpha=0.25)
    ax.axhline(0.0, color="black", linewidth=1.5)
    ax.axhline(0.2, color="black", linestyle="--", linewidth=1)
    ax.axhline(-0.2, color="black", linestyle="--", linewidth=1)

    for fitf in bkg_funcs:
        fit_mask = cat_data["fit_function"] == fitf
        fit_data = cat_data[fit_mask]
        pulls = fit_data["bias_pull"]
        #print(pulls)
        #print(cat_data[fit_mask])
        ax.errorbar(
            fit_data["toy_function"],
            fit_data["bias_pull"],
            yerr=0,
            marker="o",
            linewidth=1.8,
            markersize=5,
            capsize=2,
            label=fitf,
        )

    ax.set_xticks(bkg_funcs)
    ax.set_xticklabels(bkg_funcs, fontsize=14)
    ax.set_xlim(-0.5, len(bkg_funcs) - 0.5)
    ax.set_ylabel("Bias")
    ax.set_xlabel("Generated Toys")
    ax.set_ylim(-0.8, 0.8)

    hep.cms.label(
        data=False,
        loc=0,
        label="",
        com="13.6",
        lumi="281.7",
        ax=ax,
    )

    ax.text(
        0.05,
        0.13,
        category[0],
        transform=ax.transAxes,
        fontsize=15,
    )

    ax.legend(
        loc="upper right",
        ncol=2,
        fontsize=12,
        frameon=False,
    )

    #ax.tick_params(direction="in", top=True, right=True)
    ax.tick_params(axis="x", which="minor", bottom=False, top=False)
    #ax.minorticks_on()

    plt.tight_layout()
    plt.savefig(outputFolder + str(category[0]) + "_bias_summary.pdf")
    plt.savefig(outputFolder + str(category[0]) + "_bias_summary.png")


categories = np.unique(data["category"])
for cat in categories:
    cat_mask = data["category"]==cat
    cat_data = data[cat_mask]
    plotBiasSummary(cat_data, "BiasSummaryPlots/")