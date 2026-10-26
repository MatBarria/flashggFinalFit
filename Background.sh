#!/bin/bash
set -e

# ==========================================
# Load category configuration
# ==========================================

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

mapfile -t GGH_CATS < <(python3 "$REPO_ROOT/category_config.py" ggH)
mapfile -t VBF_CATS < <(python3 "$REPO_ROOT/category_config.py" VBF)
# ========== Background ==========
REMOVE_PREVIOUS=false

while getopts "r" opt; do
    case $opt in
        r) REMOVE_PREVIOUS=true ;;
    esac
done

if $REMOVE_PREVIOUS; then
    echo "Removing background fits"
    rm -rf Background/outdir_tutorial/*.root
    rm -rf Background/outdir_tutorial/bkgfTest-Data/*.pdf
    rm -rf Background/outdir_tutorial/bkgfTest-Data/*.png
    rm -rf Background/outdir_tutorial/bkgfTest-Data/*.txt

    echo "Removing multipdf plots fits"
    rm -rf Plots/plots_tutorial/*.p*
    exit 1
fi
cd Background

echo "[INFO] Running fTest for background"
python3 RunBackgroundScripts.py --inputConfig config_tutorial.py --mode fTestParallel
#python3 RunBackgroundScripts.py --inputConfig config_tutorial_VBF.py --mode fTestParallel

#pushd ../Plots
#CSVFILE="out_yields_$(date +%Y%m%d_%H%M%S).csv"
#for cat in "${GGH_CATS[@]}"; do
  #python3 makeMultipdfPlot.py \
    #--inputWSFile ../Background/outdir_tutorial/CMS-HGG_multipdf_${cat}.root \
    #--cat ${cat} \
    #--ext tutorial \
    #--nBins 160 \
    #--mass 125.38 \
    #--inputSignalWSFile ../Signal/outdir_packaged/CMS-HGG_sigfit_packaged_${cat}.root \
    #--outCSV ${CSVFILE}
  
#done

#for cat in "${VBF_CATS[@]}"; do
  #python3 makeMultipdfPlot.py \
    #--inputWSFile ../Background/outdir_tutorial/CMS-HGG_multipdf_${cat}.root \
    #--cat ${cat} \
    #--ext tutorial \
    #--mass 125.38 \
    #--nBins 160 \
    #--inputSignalWSFile ../Signal/outdir_packaged/CMS-HGG_sigfit_packaged_${cat}.root \
    #--outCSV ${CSVFILE}
  
#done
#popd

cd ..
