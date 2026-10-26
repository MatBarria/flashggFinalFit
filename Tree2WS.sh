# ========== Tree2WS ==========

cd Trees2WS

echo "[INFO] Running trees2ws_data for ggH"
python3 trees2ws_data.py \
  --inputConfig config_tutorial.py \
  --inputTreeFile /eos/user/z/zhguo/flashgg/ntuples/allData_combined.root \
  --applyMassCut --massCutRange 110,150

echo "[INFO] Running trees2ws for ggH signal"
python3 trees2ws.py \
  --inputConfig config_tutorial.py \
  --inputTreeFile /eos/user/z/zhguo/flashgg/ntuples/output_signal_M125_13TeV_cats_pythia8.root \
  --inputMass 125 --productionMode ggh --year combined

cd ..