#!/bin/bash
# ============================================================
# fitDYshapes.sh
#
# Wrapper around Background/bin/fitDYshapes.
#
# It takes care of the CMSSW environment (scram runtime plus
# flashggFinalFit/setup.sh, which exports ROOT_IO) and runs the
# tool from the Background directory, so that the four parameter
# files end up in Background/DYshapes/.
#
# Usage (from anywhere):
#
#   ./fitDYshapes.sh [fitDYshapes options]
#
# Examples:
#
#   ./fitDYshapes.sh                       # ggH + VBF, all categories found
#   ./fitDYshapes.sh --cats ggHcat1,ggHcat2
#   ./fitDYshapes.sh --cats VBFcat1 --no-plots
#   ./fitDYshapes.sh --cats ggHcat1 --tries 3
#   ./fitDYshapes.sh --vbf-maxratio 2      # stricter VBF outlier cut
#   ./fitDYshapes.sh --no-vbf-clean        # keep every VBF bin
#
# VBF low-statistics protection: by default every VBF bin whose
# content exceeds 3 x the content of the leftmost bin is replaced by
# a linear interpolation of its neighbours (see fitDYshapes.cpp).
# Use VBF_MAXRATIO=<r> in the environment or --vbf-maxratio /
# --no-vbf-clean on the command line to change this.
#
# The executable is built with:   cd Background && make
# ============================================================

set -e

HERE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"   # .../flashggFinalFit/Background
FFDIR="$( cd "$HERE/.." && pwd )"                          # .../flashggFinalFit
SRCDIR="$( cd "$FFDIR/.." && pwd )"                        # .../CMSSW_.../src

# --- CMSSW environment -------------------------------------------------
cd "$SRCDIR"

# the CMS setup script prints unrelated flatpak warnings, hide them
source /cvmfs/cms.cern.ch/cmsset_default.sh >/dev/null 2>&1 || true

eval `scramv1 runtime -sh`

# ROOT_IO + PYTHONPATH (also re-does the scram runtime)
source "$FFDIR/setup.sh"

# --- VBF low-statistics treatment --------------------------------------
# Outlier threshold forwarded to fitDYshapes (default: 3 x the content
# of the leftmost bin).  Command line options are parsed later, so
# --vbf-maxratio / --no-vbf-clean given by the user win over this
# default.
VBF_MAXRATIO="${VBF_MAXRATIO:-2}"

# --- run ---------------------------------------------------------------
if [ ! -x "$HERE/bin/fitDYshapes" ]; then
    echo "[ERROR] $HERE/bin/fitDYshapes not found"
    echo "        build it first:   cd $HERE && make"
    exit 1
fi

cd "$HERE"

echo "[INFO] ROOT_IO = $ROOT_IO"
echo "[INFO] running ./bin/fitDYshapes --vbf-maxratio $VBF_MAXRATIO $*"
echo

exec ./bin/fitDYshapes --vbf-maxratio "$VBF_MAXRATIO" "$@"
