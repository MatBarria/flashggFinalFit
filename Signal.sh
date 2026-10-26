# ========== Signal ==========

cd Signal

#echo "[INFO] Running fTest for signal"
#python3 RunSignalScripts.py --inputConfig config_tutorial_combined.py --mode fTest --modeOpts "--doPlots"

echo "[INFO] Running signalFit for ggH"
python3 RunSignalScripts.py --inputConfig config_tutorial_combined.py --mode signalFit --modeOpts "--doPlots --useDCB --skipVertexScenarioSplit --skipSystematics --massPoints 125 --skipBeamspotReweigh" --groupSignalFitJobsByCat

echo "[INFO] Packaging signal fits"
python3 RunPackager.py \
  --cats VBFcat1,VBFcat2,VBFcat3,VBFcat4,VBFcat5,VBFcat6,VBFcat7,VBFcat8,ggHcat1,ggHcat2,ggHcat3,ggHcat4,ggHcat5,ggHcat6 \
  --exts tutorial_combined --batch local --massPoints 125 --year combined --mergeYears

echo "[INFO] Running signal plotter"
for cat in VBFcat1 VBFcat2 VBFcat3 VBFcat4 VBFcat5 VBFcat6 VBFcat7 VBFcat8 ggHcat1 ggHcat2 ggHcat3 ggHcat4 ggHcat5 ggHcat6; do
  python3 RunPlotter.py --procs all --years combined --cats $cat --ext packaged --doFWHM 
done

cd ..