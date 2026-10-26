#!/bin/bash

LOGFILE="run_AsymptoticLimits_results_$(date +%Y%m%d_%H%M%S).log"
CSVFILE="run_AsymptoticLimits_results_$(date +%Y%m%d_%H%M%S).csv"

categories=(
    ggH
    ggHcat1
    ggHcat2
    ggHcat3
    ggHcat4
    ggHcat5
    VBF
    VBFcat1
    VBFcat2
    VBFcat3
    VBFcat4
    VBFcat5
    All
)

echo "Category,2.5%_Expected,16.0%_Expected,50.0%_Expected,84.0%_Expected,97.5%_Expected" > "$CSVFILE"

run_combine() {
    local category=$1
    local datacard="Datacard_tutorial_${category}_mu_inclusive.root"

    echo ">>> Running on $category ($datacard)" | tee -a "$LOGFILE"

    output=$(combine -M AsymptoticLimits "$datacard" -m 125 -t -1 --run blind 2>&1)

    echo "$output" >> "$LOGFILE"
    echo "" >> "$LOGFILE"

    val_2p5=$(echo "$output" | grep "Expected  2.5%" | awk '{print $5}')
    val_16=$(echo "$output" | grep "Expected 16.0%" | awk '{print $5}')
    val_50=$(echo "$output" | grep "Expected 50.0%" | awk '{print $5}')
    val_84=$(echo "$output" | grep "Expected 84.0%" | awk '{print $5}')
    val_97=$(echo "$output" | grep "Expected 97.5%" | awk '{print $5}')

    echo "$category,$val_2p5,$val_16,$val_50,$val_84,$val_97" >> "$CSVFILE"
}

for cat in "${categories[@]}"; do
    run_combine "$cat"
done

echo "All combine jobs finished."
echo "Full log: $LOGFILE"
echo "Summary CSV: $CSVFILE"
