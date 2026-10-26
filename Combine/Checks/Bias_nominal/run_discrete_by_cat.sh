#!/usr/bin/env bash
set -euo pipefail

# Usage: run_discrete_by_cat.sh <category> <toy_index> <CMSSW_release> <SCRAM_ARCH> <n_toys> <seed> <split> [--nosys]
cat="${1:?Missing category}"
toy_index="${2:?Missing generating PDF index}"
cmssw_version="${3:?Missing CMSSW release}"
requested_scram_arch="${4:?Missing SCRAM_ARCH}"
n_toys="${5:?Missing number of toys}"
seed="${6:--1}"
split="${7:?Missing split index}"
nosys_flag="${8:-}"

EXTRA_ARGS=()
if [[ "$nosys_flag" == "--nosys" ]]; then
    EXTRA_ARGS+=(--toysNoSystematics --statOnly)
fi


work_dir="$(pwd -P)"
mkdir -p BiasResults

for path in "Datacard_toy_${cat}.root" "Datacard_fit_${cat}.root" \
            RunBiasStudy_discrete.py combine bias_headers.tar.gz \
            libHiggsAnalysisCombinedLimit.so libflashggFinalFitDYCore.so; do
  [[ -s "$path" ]] || { echo "Missing/empty input: $path" >&2; exit 2; }
done

echo "Category=$cat toyIndex=$toy_index toys=$n_toys seed=$seed split=$split"
source /cvmfs/cms.cern.ch/cmsset_default.sh
export SCRAM_ARCH="$requested_scram_arch"
scramv1 project CMSSW "$cmssw_version"
tar -xzf "$work_dir/bias_headers.tar.gz" -C "$cmssw_version/src"
# Verify spline file exists on the worker
SPLINE_FILE="$cmssw_version/src/flashggFinalFit/Background/DYshapes/fewz_spline.root"

if [[ ! -s "$SPLINE_FILE" ]]; then
    echo "[ERROR] Missing spline file: $SPLINE_FILE" >&2
    exit 2
fi

echo "[INFO] Found spline file: $SPLINE_FILE"
test -f "$cmssw_version/src/HiggsAnalysis/CombinedLimit/interface/RooMultiPdf.h"
cp "Datacard_toy_${cat}.root" "Datacard_fit_${cat}.root" \
   RunBiasStudy_discrete.py combine "$cmssw_version/src/"
chmod +x "$cmssw_version/src/combine"
cd "$cmssw_version/src"
eval "$(scram runtime -sh)"
export BIAS_LIBRARY_DIR="$work_dir"
export LD_LIBRARY_PATH="$work_dir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export ROOT_INCLUDE_PATH="$PWD${ROOT_INCLUDE_PATH:+:$ROOT_INCLUDE_PATH}"
export PATH="$PWD:$PATH"

# Fail fast if dictionaries, ROOT workspaces, or PDFs are unavailable.
python3 RunBiasStudy_discrete.py \
  -d "Datacard_toy_${cat}.root" -w "Datacard_fit_${cat}.root" \
  -b "$cat" --listFunctions

csv="bias_results_${cat}.csv"
pull_csv="bias_pulls_${cat}.csv"
python3 RunBiasStudy_discrete.py \
  -d "Datacard_toy_${cat}.root" \
  -w "Datacard_fit_${cat}.root" \
  -b "$cat" --toyIndex "$toy_index" \
  -n "$n_toys" -s "$seed" \
  --outCSV "$csv" \
  --outPullCSV "$pull_csv" \
  "${EXTRA_ARGS[@]}"

cp -r BiasToys BiasFits BiasPlots "$csv" "$pull_csv" "$work_dir/BiasResults/"
echo "Complete: $cat generating PDF $toy_index -> $work_dir/BiasResults/$csv"
