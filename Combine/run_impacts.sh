#!/bin/bash

set -euo pipefail

# ============================================================
# Expected / Asimov impacts
# ============================================================

WORKSPACE="Datacard_tutorial_All_mu_inclusive.root"
MASS=125
IMPACTS_DIR="Impacts"
NCPU=4

COMMON_OPTS="\
--cminDefaultMinimizerStrategy 0 \
--X-rtd MINIMIZER_freezeDisassociatedParams \
--X-rtd MINIMIZER_multiMin_hideConstants \
--X-rtd MINIMIZER_multiMin_maskConstraints \
--X-rtd MINIMIZER_multiMin_maskChannels=2"

# Check input
if [ ! -f "${WORKSPACE}" ]; then
    echo "[ERROR] Cannot find ${WORKSPACE}"
    exit 1
fi

# Prepare impacts directory
echo " Preparing impacts directory"

rm -rf "${IMPACTS_DIR}"
mkdir -p "${IMPACTS_DIR}"

cp "${WORKSPACE}" "${IMPACTS_DIR}/"
cd "${IMPACTS_DIR}"

# Initial fit
echo
echo " Running initial fit"

combineTool.py -M Impacts \
    -d "${WORKSPACE}" \
    -m "${MASS}" \
    --doInitialFit \
    --robustFit 1 \
    -t -1 \
    --expectSignal 1 \
    --setParameters r=1 \
    --setParameterRanges r=0,2 \
    ${COMMON_OPTS} \
    --freezeParameters MH \
    2>&1 | tee impacts_initialFit.log

# ------------------------------------------------------------
# Individual impact fits
# ------------------------------------------------------------

echo
echo "========================================"
echo " Running impact fits"
echo "========================================"

combineTool.py -M Impacts \
    -d "${WORKSPACE}" \
    -m "${MASS}" \
    --doFits \
    --robustFit 1 \
    -t -1 \
    --expectSignal 1 \
    --setParameters r=1 \
    --setParameterRanges r=0,2 \
    ${COMMON_OPTS} \
    --freezeParameters MH \
    --parallel "${NCPU}" \
    2>&1 | tee impacts_doFits.log

# ------------------------------------------------------------
# Collect impacts
# ------------------------------------------------------------

echo
echo " Collecting impacts"

combineTool.py -M Impacts \
    -d "${WORKSPACE}" \
    -m "${MASS}" \
    -o impacts.json

# Plot WITH background PDF parameters
echo
echo " Plotting impacts WITH PDF parameters"

plotImpacts.py \
    -i impacts.json \
    -o impacts_withPdf

# Remove background PDF/model parameters
echo
echo " Removing background PDF parameters"

python3 ../../Plots/correctImpacts.py \
    --impactsJson impacts.json \
    --dropBkgModelParams \
    --frozenParam MH

# This produces: impacts_corrected_dropBkgModelParams.json

# Plot WITHOUT background PDF parameters
echo " Plotting impacts WITHOUT PDF parameters"

plotImpacts.py \
    -i impacts_corrected_dropBkgModelParams.json \
    -o impacts_withoutPdf

# Done
echo
echo " Done"
echo
echo "Outputs:"
echo "  Impacts/impacts.json"
echo "  Impacts/impacts_withPdf.pdf"
echo "  Impacts/impacts_withoutPdf.pdf"
