#!/usr/bin/env python3

import argparse

# Total number of categories
N_GGH = 5
N_VBF = 5

# First category index
GGH_FIRST = 1
VBF_FIRST = 1


def get_categories(channel):

    config = {
        "ggH": (GGH_FIRST, N_GGH),
        "VBF": (VBF_FIRST, N_VBF),
    }

    first, n_cats = config[channel]

    return [
        f"{channel}cat{i}"
        for i in range(first, first + n_cats)
    ]


def get_all_categories():

    return (
        get_categories("ggH")
        + get_categories("VBF")
    )


def get_workspace_categories():

    return (
        ["ggH"]
        + get_categories("ggH")
        + ["VBF"]
        + get_categories("VBF")
        + ["All"]
    )


# ==========================================
# Bash interface
# ==========================================

if __name__ == "__main__":

    parser = argparse.ArgumentParser()

    parser.add_argument(
        "mode",
        choices=["ggH", "VBF", "all", "workspace"],
    )

    args = parser.parse_args()

    if args.mode == "ggH":
        cats = get_categories("ggH")

    elif args.mode == "VBF":
        cats = get_categories("VBF")

    elif args.mode == "workspace":
        cats = get_workspace_categories()

    else:
        cats = get_all_categories()

    for cat in cats:
        print(cat)
