#!/bin/bash
set -e

SKIP_SYSTEMATICS=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --nosys|-nosys)
            SKIP_SYSTEMATICS=true
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
    shift
done


cmsenv
source ../../../setup.sh

rm -f Datacard_tutorial*.root


# Load categories from configuration
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
mapfile -t CATS < <(
    python3 "$REPO_ROOT/category_config.py" all
)

echo "===== Create toy background ====="
cd ../../..
bash Background_toy.sh > check_toy.txt 2>&1
#bash Background_toy.sh
cd Datacard
echo "[INFO] Generating Datacards"
if $SKIP_SYSTEMATICS; then
    python3 generate_datacards.py --nosys
else
    python3 generate_datacards.py
fi
cd ../Combine
bash copy.sh
bash run_workspace.sh
cd Checks/Bias_nominal/

for cat in "${CATS[@]}"; do
    cp -v "../../Datacard_tutorial_${cat}_mu_inclusive.root" \
       "./Datacard_tutorial_${cat}_mu_inclusive_toy.root"
done

echo "===== Create nominal background ====="
cd ../../..
bash Background.sh
cd Datacard
echo "[INFO] Generating Datacards"
if $SKIP_SYSTEMATICS; then
    python3 generate_datacards.py --nosys
else
    python3 generate_datacards.py
fi
cd ../Combine
bash copy.sh
bash run_workspace.sh
cd Checks/Bias_nominal/

for cat in "${CATS[@]}"; do
    cp -v "../../Datacard_tutorial_${cat}_mu_inclusive.root" \
       "./Datacard_tutorial_${cat}_mu_inclusive.root"
done

echo "===== Finished ====="
