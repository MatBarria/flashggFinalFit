#!/bin/bash
set -e

# Get version from command-line argument
VERSION="${1:?ERROR: Please provide a version name. Usage: bash save_fits.sh <version>}"

echo "Organizing outputs into ${VERSION}..."

# Move only if the source exists
move_if_exists() {
    local source="$1"
    local destination="$2"

    if [[ -e "$source" ]]; then
        mkdir -p "$destination"
        mv -v "$source" "$destination/"
    else
        echo "Skipping: $source (does not exist)"
    fi
}

# Signal
move_if_exists "Signal/outdir_packaged" "Signal/${VERSION}"
move_if_exists "Signal/outdir_tutorial_combined" "Signal/${VERSION}"

# Background
move_if_exists "Background/outdir_tutorial" "Background/${VERSION}"

# Plots
move_if_exists "Plots/plots_toys" "Plots/${VERSION}"
move_if_exists "Plots/plots_tutorial" "Plots/${VERSION}"

# Datacards
for file in Datacard/Datacard_tutorial*; do
    move_if_exists "$file" "Datacard/${VERSION}"
done

echo "Done."
echo "Outputs moved to ${VERSION}"
