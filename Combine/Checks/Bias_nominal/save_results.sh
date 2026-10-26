#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   bash save_results.sh                # archive to Oct19_nosys/
#   bash save_results.sh Another_name   # archive to Another_name/
#
# Run only after the Condor jobs using condor_discrete/ have finished.
# A move on the same filesystem does not free disk quota or create an offsite backup.

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
archive="${1:-Oct19_nosys}"

# Accept a directory name, not an absolute/relative path to somewhere else.
if [[ ! "$archive" =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]]; then
    echo "[ERROR] Use a simple archive directory name, e.g. Oct19_nosys" >&2
    exit 1
fi

if [[ -e "$archive" && ! -d "$archive" ]]; then
    echo "[ERROR] $archive exists but is not a directory." >&2
    exit 1
fi
if [[ -d "$archive" ]] && [[ -n "$(find "$archive" -mindepth 1 -print -quit)" ]]; then
    echo "[ERROR] $archive already contains files. Refusing to overwrite/mix studies." >&2
    echo "        Choose a new name: bash save_results.sh Oct20_nosys" >&2
    exit 1
fi

# Required to identify the complete study and preserve the per-toy data.
for required in condor_discrete bias_results_merged.csv bias_pulls_merged.csv; do
    if [[ ! -e "$required" ]]; then
        echo "[ERROR] Missing required result: $required" >&2
        echo "        Run this script from Combine/Checks/Bias_nominal after merging." >&2
        exit 1
    fi
done

mkdir -p "$archive/scripts"

# Move output datasets and figures; missing optional plot directories are okay.
for item in \
    condor_discrete \
    bias_results_merged.csv \
    bias_pulls_merged.csv \
    BiasPlots_merged \
    BiasPlots_summary; do
    if [[ -e "$item" ]]; then
        mv -v -- "$item" "$archive/"
    else
        echo "[WARN] Not present, skipping: $item"
    fi
done

# Preserve the scripts, but keep working copies in Bias_nominal/.
for file in \
    merge_discrete_bias_results.py \
    plot_bias_pull_medians.py \
    create_bias_jobs.py \
    create_discrete_bias_jobs.py \
    RunBiasStudy_discrete.py \
    run_discrete_by_cat.sh \
    category_config.py \
    run_bias_test.sh \
    plot_bias_by_category_discrete.py \
    save_results.sh; do
    if [[ -f "$file" ]]; then
        cp -p -v -- "$file" "$archive/scripts/"
    fi
done

helper='../../../Plots/utils/helper.py'
if [[ -f "$helper" ]]; then
    cp -p -v -- "$helper" "$archive/scripts/helper.py"
else
    echo "[WARN] Plotting helper not found: $helper"
fi

echo
echo "[OK] Results archived in: $(pwd)/$archive"
du -sh -- "$archive"
printf '\nArchive contents:\n'
ls -lah -- "$archive"
echo
echo '[NOTE] This is a move within the current filesystem, not a separate backup.'
