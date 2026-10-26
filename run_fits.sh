#!/bin/bash
set -e

# ==========================================
# Load category configuration
# ==========================================

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

mapfile -t GGH_CATS < <(python3 "$REPO_ROOT/category_config.py" ggH)
mapfile -t VBF_CATS < <(python3 "$REPO_ROOT/category_config.py" VBF)

ALL_CATS=("${VBF_CATS[@]}" "${GGH_CATS[@]}")

# Comma-separated list for RunPackager.py
PACKAGER_CATS=$(IFS=,; echo "${ALL_CATS[*]}")

REMOVE_PREVIOUS=false
SKIP_SYSTEMATICS=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        -r)
            REMOVE_PREVIOUS=true
            ;;
        --nosys|-nosys)
            SKIP_SYSTEMATICS=true
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
    shift
done


if $REMOVE_PREVIOUS; then
    echo "Removing previous results..."
    echo "Removing previous workspaces"
    rm -rfv ${ROOT_IO}/tuples/split_tuples/ggH/ws/*.root
    rm -rfv ${ROOT_IO}/tuples/split_tuples/ggH/ws_GG2H/*.root
    rm -rfv ${ROOT_IO}/tuples/split_tuples/VBF/ws/*.root
    rm -rfv ${ROOT_IO}/tuples/split_tuples/VBF/ws_VBF/*.root

    echo "Removing background fits"
    rm -rfv Background/outdir_tutorial/*.root
    rm -rfv Background/outdir_tutorial/bkgfTest-Data/*.pdf
    rm -rfv Background/outdir_tutorial/bkgfTest-Data/*.png
    rm -rfv Background/outdir_tutorial/bkgfTest-Data/*.txt

    echo "Removing datacards"
    rm -rfv Combine/Datacard*
    rm -rfv Datacard/Datacard*
    
    echo "Removing signal fits"
    rm -rfv Signal/outdir_tutorial_combined/signalFit/Plots/*.p*
    rm -rfv Signal/outdir_tutorial_combined/signalFit/output/*.root
    rm -rfv Signal/outdir_packaged/*.root
    rm -rfv Signal/outdir_packaged/Plots/*.p*

    echo "Removing output plots"
    rm -rfv Plots/plots_tutorial/*.p*
    exit 1
fi


SKIP_BLOCK=false
# ========== Tree2WS ==========
if ! $SKIP_BLOCK; then
  cd Trees2WS

  echo "[INFO] Running trees2ws_data for ggH"
  python3 trees2ws_data.py \
    --inputConfig config_tutorial.py \
    --inputTreeFile ${ROOT_IO}/tuples/split_tuples/allData_combined.root \
    --applyMassCut --massCutRange 100,180

  #echo "[INFO] Running trees2ws_data for VBF"
  #python3 trees2ws_data.py \
  #  --inputConfig config_tutorial.py \
  #  --inputTreeFile ${ROOT_IO}/tuples/split_tuples/VBF/allData_combined.root \
  #  --applyMassCut --massCutRange 100,180

  echo "[INFO] Running trees2ws for ggH signal"
  python3 trees2ws.py \
    --inputConfig config_tutorial.py \
    --inputTreeFile ${ROOT_IO}/tuples/split_tuples/ggH/output_signal_M125_13TeV_cats_pythia8.root \
    --inputMass 125 --productionMode ggh --year combined

  echo "[INFO] Running trees2ws for VBF signal"
  python3 trees2ws.py \
    --inputConfig config_tutorial.py \
    --inputTreeFile ${ROOT_IO}/tuples/split_tuples/VBF/output_signal_M125_13TeV_cats_pythia8.root \
    --inputMass 125 --productionMode vbf --year combined
  
  cd ..
  python3 cp_workspaces.py

  # ========== Signal ==========

  cd Signal

  #echo "[INFO] Running fTest for signal"
  #python3 RunSignalScripts.py --inputConfig config_tutorial_combined.py --mode fTest --modeOpts "--doPlots"

  echo "[INFO] Running signalFit for ggH"
  python3 RunSignalScripts.py --inputConfig config_tutorial_combined.py --mode signalFit --modeOpts "--doPlots --useDCB --skipVertexScenarioSplit --skipSystematics --massPoints 125 --skipBeamspotReweigh" --groupSignalFitJobsByCat

  echo "[INFO] Running signalFit for VBF"
  python3 RunSignalScripts.py --inputConfig config_tutorial_combined_VBF.py --mode signalFit --modeOpts "--doPlots --useDCB --skipVertexScenarioSplit --skipSystematics --massPoints 125 --skipBeamspotReweigh" --groupSignalFitJobsByCat

  echo "[INFO] Packaging signal fits"
  #python3 RunPackager.py \
    #--cats VBFcat1,VBFcat2,VBFcat3,VBFcat4,VBFcat5,VBFcat6,ggHcat1,ggHcat2,ggHcat3,ggHcat4,ggHcat5,ggHcat6 \
    #--exts tutorial_combined --batch local --massPoints 125 --year combined --mergeYears
  echo "[INFO] Packaging signal fits"

  python3 RunPackager.py \
    --cats "$PACKAGER_CATS" \
    --exts tutorial_combined \
    --batch local \
    --massPoints 125 \
    --year combined \
    --mergeYears

  echo "[INFO] Running signal plotter"
  #for cat in VBFcat1 VBFcat2 VBFcat3 VBFcat4 VBFcat5 VBFcat6 ggHcat1 ggHcat2 ggHcat3 ggHcat4 ggHcat5 ggHcat6; do
    #python3 RunPlotter.py --procs all --years combined --cats $cat --ext packaged --doFWHM 
  #done

  for cat in "${ALL_CATS[@]}"; do
    python3 RunPlotter.py \
            --procs all \
            --years combined \
            --cats "$cat" \
            --ext packaged \
            --doFWHM
  done
  cd ..

  # ========== Background ==========

  cd Background

  echo "[INFO] Running MC background"
  bash fitDYshapes.sh

  echo "[INFO] Running fTest for background"
  python3 RunBackgroundScripts.py --inputConfig config_tutorial.py --mode fTestParallel
  #python3 RunBackgroundScripts.py --inputConfig config_tutorial_VBF.py --mode fTestParallel
else
  cd Background
fi

pushd ../Plots
CSVFILE="out_yields_$(date +%Y%m%d_%H%M%S).csv"
#for cat in ggHcat1 ggHcat2 ggHcat3 ggHcat4 ggHcat5 ggHcat6; do
for cat in "${GGH_CATS[@]}"; do
  python3 makeMultipdfPlot.py \
   --inputWSFile ../Background/outdir_tutorial/CMS-HGG_multipdf_${cat}.root \
    --cat ${cat} \
    --ext tutorial \
    --nBins 160 \
    --mass 125.38 \
    --inputSignalWSFile ../Signal/outdir_packaged/CMS-HGG_sigfit_packaged_${cat}.root \
    --outCSV ${CSVFILE}
  
done

#for cat in VBFcat1 VBFcat2 VBFcat3 VBFcat4 VBFcat5 VBFcat6; do
for cat in "${VBF_CATS[@]}"; do
  python3 makeMultipdfPlot.py \
    --inputWSFile ../Background/outdir_tutorial/CMS-HGG_multipdf_${cat}.root \
    --cat ${cat} \
    --ext tutorial \
    --mass 125.38 \
    --nBins 160 \
    --inputSignalWSFile ../Signal/outdir_packaged/CMS-HGG_sigfit_packaged_${cat}.root \
    --outCSV ${CSVFILE}
  
done

#for cat in ggHcat1 ggHcat2 ggHcat3 ggHcat4 ggHcat5 ggHcat6; do
for cat in "${GGH_CATS[@]}"; do
  python3 makeMultipdfPlot_hepmpl.py \
   --inputWSFile ../Background/outdir_tutorial/CMS-HGG_multipdf_${cat}.root \
    --cat ${cat} \
    --ext tutorial \
    --nBins 80 \
    --mass 125.38 \
    --inputSignalWSFile ../Signal/outdir_packaged/CMS-HGG_sigfit_packaged_${cat}.root \
    --outCSV ${CSVFILE}
  
done

#for cat in VBFcat1 VBFcat2 VBFcat3 VBFcat4 VBFcat5 VBFcat6; do
for cat in "${VBF_CATS[@]}"; do
  python3 makeMultipdfPlot_hepmpl.py \
    --inputWSFile ../Background/outdir_tutorial/CMS-HGG_multipdf_${cat}.root \
    --cat ${cat} \
    --ext tutorial \
    --mass 125.38 \
    --nBins 80 \
    --inputSignalWSFile ../Signal/outdir_packaged/CMS-HGG_sigfit_packaged_${cat}.root \
    --outCSV ${CSVFILE}
  
done

popd

cd ../Datacard
echo "[INFO] Generating Datacards"

if $SKIP_SYSTEMATICS; then
    python3 generate_datacards.py --nosys
else
    python3 generate_datacards.py
fi

cd ../
cd Combine

echo "[INFO] Copying Signal, Data, and Datacards into combine folder"
bash copy.sh

echo "[INFO] Creating workspaces for Combine"
bash run_workspace.sh

echo "[INFO] Running asymptotic limits"
bash run_asym.sh

echo "[INFO] Running significance"
bash run_sig.sh  
