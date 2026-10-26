#!/bin/bash

set -euo pipefail

WORKSPACE="Datacard_tutorial_All_mu_inclusive.root"
MASS=125
OUTDIR="Likelihood"

# Number of grid points in r
POINTS=20

RMIN=0
RMAX=2

COMMON_OPTS="\
--cminDefaultMinimizerStrategy 0 \
--X-rtd MINIMIZER_freezeDisassociatedParams \
--X-rtd MINIMIZER_multiMin_hideConstants \
--X-rtd MINIMIZER_multiMin_maskConstraints \
--X-rtd MINIMIZER_multiMin_maskChannels=2"

# H -> mumu theory nuisances
THEORY_NUIS="BR_hmm,thu_QCDscale_ggH,thu_QCDscale_qqH,thu_pdf_Higgs_ggH,thu_pdf_Higgs_qqH,alphaS_ggH,alphaS_qqH"

# Check workspace
if [ ! -f "${WORKSPACE}" ]; then
    echo "[ERROR] Cannot find ${WORKSPACE}"
    exit 1
fi

# Prepare directory
echo " Preparing likelihood directory"

rm -rf "${OUTDIR}"
mkdir -p "${OUTDIR}"

cp "${WORKSPACE}" "${OUTDIR}/"
cd "${OUTDIR}"

echo
echo " Running expected best fit"

combine -M MultiDimFit \
    -d "${WORKSPACE}" \
    -m "${MASS}" \
    -n _bestfit_syst_r \
    --redefineSignalPOIs r \
    --algo none \
    --saveWorkspace \
    --saveNLL \
    -t -1 \
    --expectSignal 1 \
    --setParameterRanges r=${RMIN},${RMAX} \
    ${COMMON_OPTS} \
    --freezeParameters MH \
    2>&1 | tee bestfit.log

BESTFIT="higgsCombine_bestfit_syst_r.MultiDimFit.mH${MASS}.root"

if [ ! -f "${BESTFIT}" ]; then
    echo "[ERROR] Best-fit ROOT file was not produced"
    exit 1
fi

echo
echo " Running full-systematics scan"

combine -M MultiDimFit \
    -d "${BESTFIT}" \
    -m "${MASS}" \
    -n _profile1D_syst_r \
    --snapshotName MultiDimFit \
    --redefineSignalPOIs r \
    --algo grid \
    --points "${POINTS}" \
    --saveNLL \
    --saveSpecifiedNuis all \
    -t -1 \
    --expectSignal 1 \
    --setParameterRanges r=${RMIN},${RMAX} \
    ${COMMON_OPTS} \
    --freezeParameters MH \
    2>&1 | tee profile1D_syst.log

# 3. Remove theory uncertainties

echo
echo "========================================"
echo " Running scan without theory uncertainties"
echo "========================================"

combine -M MultiDimFit \
    -d "${BESTFIT}" \
    -m "${MASS}" \
    -n _profile1D_removeTHU_r \
    --snapshotName MultiDimFit \
    --redefineSignalPOIs r \
    --algo grid \
    --points "${POINTS}" \
    --saveNLL \
    --saveSpecifiedNuis all \
    -t -1 \
    --expectSignal 1 \
    --setParameterRanges r=${RMIN},${RMAX} \
    ${COMMON_OPTS} \
    --freezeParameters MH,${THEORY_NUIS} \
    2>&1 | tee profile1D_removeTHU.log

# Freeze all constrained nuisance parameters at their
# post-fit values.

echo
echo " Running stat-only scan"

combine -M MultiDimFit \
    -d "${BESTFIT}" \
    -m "${MASS}" \
    -n _profile1D_statonly_r \
    --snapshotName MultiDimFit \
    --redefineSignalPOIs r \
    --algo grid \
    --points "${POINTS}" \
    --saveNLL \
    --saveSpecifiedNuis all \
    -t -1 \
    --expectSignal 1 \
    --setParameterRanges r=${RMIN},${RMAX} \
    ${COMMON_OPTS} \
    --freezeParameters MH,allConstrainedNuisances \
    2>&1 | tee profile1D_statonly.log

# Clean the root files names

cp \
    "higgsCombine_profile1D_syst_r.MultiDimFit.mH${MASS}.root" \
    profile1D_syst_r.root

cp \
    "higgsCombine_profile1D_removeTHU_r.MultiDimFit.mH${MASS}.root" \
    profile1D_removeTHU_r.root

cp \
    "higgsCombine_profile1D_statonly_r.MultiDimFit.mH${MASS}.root" \
    profile1D_statonly_r.root

echo " Plotting likelihood scans"

plot1DScan.py \
    profile1D_syst_r.root \
    --POI r \
    --main-label "Syst" \
    --main-color 1 \
    --others \
        profile1D_removeTHU_r.root:"Remove THU":2 \
        profile1D_statonly_r.root:"Stat-only":4 \
    --breakdown Theory,Exp,Stat \
    --y-cut 50 \
    --y-max 20 \
    --logo-sub Internal \
    -o likelihood_r


echo
echo " Likelihood scans finished"
echo
echo "Outputs:"
echo "  ${OUTDIR}/likelihood_r.pdf"
echo "  ${OUTDIR}/profile1D_syst_r.root"
echo "  ${OUTDIR}/profile1D_removeTHU_r.root"
echo "  ${OUTDIR}/profile1D_statonly_r.root"
echo "  ${OUTDIR}/${BESTFIT}"
echo
echo "Logs:"
echo "  ${OUTDIR}/bestfit.log"
echo "  ${OUTDIR}/profile1D_syst.log"
echo "  ${OUTDIR}/profile1D_removeTHU.log"
echo "  ${OUTDIR}/profile1D_statonly.log"
