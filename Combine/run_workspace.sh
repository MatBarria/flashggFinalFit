#!/usr/bin/env bash
set -euo pipefail

LOGFILE="${LOGFILE:-text2workspace.log}"

#categories=(
  #ggH ggHcat1 ggHcat2 ggHcat3 ggHcat4 ggHcat5 ggHcat6
  #VBF VBFcat1 VBFcat2 VBFcat3 VBFcat4 VBFcat5 VBFcat6 All
#)

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mapfile -t categories < <(
    python3 "$REPO_ROOT/category_config.py" workspace
)


: > "$LOGFILE"

run_text2workspace() {
  local category="$1"

  echo ">>> [$category] START" | tee -a "$LOGFILE"

  if python3 RunText2Workspace.py \
      --mode mu_inclusive \
      --batch local \
      --ext "_tutorial_${category}" \
      >> "$LOGFILE" 2>&1; then

    echo ">>> [$category] OK" | tee -a "$LOGFILE"
  else
    echo ">>> [$category] FAIL" | tee -a "$LOGFILE"
  fi

  echo "" >> "$LOGFILE"
}

for cat in "${categories[@]}"; do
  run_text2workspace "$cat"
done

echo "All Text2Workspace jobs finished."
echo "Log file: $LOGFILE"

