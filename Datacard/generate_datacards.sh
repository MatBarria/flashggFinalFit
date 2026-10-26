#!/bin/bash

set -e

run_datacard() {
    local CAT=$1
    local CHANNEL=$2
    local WS_PATH=$3

    echo "---------------------------- $CHANNEL: $CAT ----------------------------"
    echo "$WS_PATH"
    python3 RunYields.py \
        --inputWSDirMap combined="$WS_PATH" \
        --cats "$CAT" \
        --procs auto \
        --ext tutorial \
        --mergeYears \
        --skipCOWCorr \
        --batch local \
        --queue espresso

    python3 makeDatacard.py \
        --ext tutorial \
        --years combined \
        --prune \
        --doTrueYield \
        --skipCOWCorr \
        --doMCStatUncertainty \
        --saveDataFrame \
        --output Datacard_tutorial

    cp Datacard_tutorial.txt Datacard_tutorial_${CAT}.txt
    cp Datacard_tutorial.pkl Datacard_tutorial_${CAT}.pkl

   # Data cards are generated using all the yield .pkl files so its necesary to clean the directory  
    rm -f yields_tutorial/*.pkl
}

run_combined_datacard() {
    local CATS=$1
    local CHANNEL=$2
    local WS_PATH=$3

    echo "---------------------------- $CHANNEL: Combined ----------------------------"
    echo "==========Cats: $CATSi ============="

    python3 RunYields.py \
        --inputWSDirMap combined="$WS_PATH" \
        --cats "$CATS" \
        --procs auto \
        --ext tutorial \
        --mergeYears \
        --skipCOWCorr \
        --batch local \
        --queue espresso

    python3 makeDatacard.py \
        --ext tutorial \
        --years combined \
        --prune \
        --doTrueYield \
        --skipCOWCorr \
        --doMCStatUncertainty \
        --saveDataFrame \
        --output Datacard_tutorial

    cp Datacard_tutorial.txt Datacard_tutorial_${CHANNEL}.txt
    cp Datacard_tutorial.pkl Datacard_tutorial_${CHANNEL}.pkl

}

### ggH
GGH_CATS=("ggHcat1" "ggHcat2" "ggHcat3" "ggHcat4" "ggHcat5" "ggHcat6")
GGH_PATH="${ROOT_IO}/tuples/split_tuples/ggH/ws_GG2H"
echo "---------------------------- ggH -----------------------------------"
for cat in "${GGH_CATS[@]}"; do
    run_datacard $cat "ggH" $GGH_PATH
done

run_combined_datacard "$(IFS=, ; echo "${GGH_CATS[*]}")" "ggH" $GGH_PATH

# Save the ggH yields for the final Datacard
mkdir -p yields_tutorial/ggH
for cat in "${GGH_CATS[@]}"; do
    mv -v "yields_tutorial/${cat}.pkl" "yields_tutorial/ggH/${cat}.pkl"
done
rm -f yields_tutorial/*.pkl

### VBF
VBF_CATS=("VBFcat1" "VBFcat2" "VBFcat3" "VBFcat4" "VBFcat5" "VBFcat6")
VBF_PATH="${ROOT_IO}/tuples/split_tuples/VBF/ws_VBF"
echo "---------------------------- VBF -----------------------------------"
for cat in "${VBF_CATS[@]}"; do
    run_datacard $cat "VBF" $VBF_PATH
done
run_combined_datacard "$(IFS=, ; echo "${VBF_CATS[*]}")" "VBF" $VBF_PATH

### Final Combined
echo "---------------------------- Combined -----------------------------------"
# Move back ggh cats to yields_tutorial/
for cat in "${GGH_CATS[@]}"; do
    mv -v "yields_tutorial/ggH/${cat}.pkl" "yields_tutorial/${cat}.pkl"
done

python3 makeDatacard.py \
    --ext tutorial \
    --years combined \
    --prune \
    --doTrueYield \
    --skipCOWCorr \
    --doMCStatUncertainty \
    --saveDataFrame \
    --output Datacard_tutorial

# Final cleanup
rm -f yields_tutorial/*.pkl


