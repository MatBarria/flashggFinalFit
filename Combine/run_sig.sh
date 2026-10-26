#!/bin/bash

LOGFILE="run_Signifiance_results_$(date +%Y%m%d_%H%M%S).log"
CSVFILE="run_Signifiance_results_$(date +%Y%m%d_%H%M%S).csv"

# Load categories
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

mapfile -t GGH_CATS < <(python3 "$REPO_ROOT/category_config.py" ggH)
mapfile -t VBF_CATS < <(python3 "$REPO_ROOT/category_config.py" VBF)

categories=(
    ggH
    "${GGH_CATS[@]}"
    VBF
    "${VBF_CATS[@]}"
    All
)


echo "Category,Significance" > "$CSVFILE"

run_combine() {
    local category=$1
    local datacard="Datacard_tutorial_${category}_mu_inclusive.root"

    echo ">>> Running on $category ($datacard)" | tee -a "$LOGFILE"

    output=$(combine -M Significance "$datacard" -m 125 -t -1 --expectSignal 1 --setParameterRanges MH=120,130 -v 3 --freezeParameters MH 2>&1)

    echo "$output" >> "$LOGFILE"
    echo "" >> "$LOGFILE"

    val_sig=$(echo "$output" | grep "Significance:" | awk '{print $2}')

    echo "$category,$val_sig" >> "$CSVFILE"
}

for cat in "${categories[@]}"; do
    run_combine "$cat"
done

echo "All combine jobs finished."
echo "Full log: $LOGFILE"
echo "Summary CSV: $CSVFILE"
cat $CSVFILE
