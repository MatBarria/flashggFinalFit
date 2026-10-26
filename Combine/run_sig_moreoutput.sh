#!/bin/bash

# =========================
# Output files
# =========================
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOGFILE="run_Significance_results_${TIMESTAMP}.log"
CSVFILE="run_Significance_results_${TIMESTAMP}.csv"

# =========================
# Categories
# =========================
categories=(
    ggH
    ggHcat1
    ggHcat2
    ggHcat3
    ggHcat4
    ggHcat5
    ggHcat6
)

# =========================
# CSV header
# =========================
echo "Category,Significance,q0,r_bestfit,pdfindex" > "$CSVFILE"

# =========================
# Function to run combine
# =========================
run_combine() {

    local category=$1
    local datacard="Datacard_tutorial_${category}_mu_inclusive.root"

    echo "========================================" | tee -a "$LOGFILE"
    echo ">>> Running on $category" | tee -a "$LOGFILE"
    echo "========================================" | tee -a "$LOGFILE"

    output=$(combine -M Significance "$datacard" \
        -m 125 \
        -t -1 \
        --expectSignal 1 \
        -v 1 \
        2>&1)

    echo "$output" | tee -a "$LOGFILE"

    val_sig=$(echo "$output" | grep -i "significance" | tail -1 | awk '{print $NF}')
    val_q0=$(echo "$output" | grep -i "q0" | tail -1 | awk '{print $NF}')

    val_sig=${val_sig:-NA}
    val_q0=${val_q0:-NA}

    echo "$category,$val_sig,$val_q0" >> "$CSVFILE"
}

# =========================
# Run all categories
# =========================
for cat in "${categories[@]}"; do
    run_combine "$cat"
done

# =========================
# Final message
# =========================
echo "========================================"
echo "All combine jobs finished."
echo "Full log: $LOGFILE"
echo "Summary CSV: $CSVFILE"
echo "========================================"