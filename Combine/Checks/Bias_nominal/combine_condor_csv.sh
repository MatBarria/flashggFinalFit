#!/bin/bash

output="combined.csv"
first=true

#find . -name "condor/BiasCondor*/bias_results_*_2*_2*.csv" | sort | while read file; do
find ./condor -type f -name "bias_results_*_2*_2*.csv" | sort | while IFS= read -r file; do
    if $first; then
        cat "$file" > "$output"
        first=false
    else
        tail -n +2 "$file" >> "$output"
    fi
done