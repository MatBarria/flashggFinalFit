#!/usr/bin/env python3
import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Iterable, List

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from category_config import get_categories


def run(cmd: List[str]) -> None:
    """Run a command, print it, and raise if it fails."""
    print("\n$ " + " ".join(cmd))
    subprocess.run(cmd, check=True)


def ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def rm_glob(pattern: str) -> None:
    """Remove files matching a glob pattern (no error if none)."""
    for p in Path().glob(pattern):
        if p.is_file():
            p.unlink()


def run_yields(ws_path: str, cats: str) -> None:
    run(
        [
            "python3",
            "RunYields.py",
            "--inputWSDirMap",
            f"combined={ws_path}",
            "--cats",
            cats,
            "--procs",
            "auto",
            "--ext",
            "tutorial",
            "--mergeYears",
            "--skipCOWCorr",
            "--batch",
            "local",
            "--queue",
            "espresso",
        ]
    )


def make_datacard(skip_systematics: bool = False) -> None:
    cmd = [
        "python3",
        "makeDatacard.py",
        "--ext",
        "tutorial",
        "--years",
        "combined",
        "--prune",
        "--doTrueYield",
        "--skipCOWCorr",
        "--saveDataFrame",
        "--output",
        "Datacard_tutorial",
    ]

    if not skip_systematics:
        cmd.extend(
            [
                "--doMCStatUncertainty",
                "--doSystematic",
            ]
        )

    run(cmd)


def run_datacard(
    cat: str, channel: str, ws_path: str, skip_systematics: bool = False
) -> None:
    print(f"---------------------------- {channel}: {cat} ----------------------------")
    print(ws_path)

    run_yields(ws_path=ws_path, cats=cat)
    make_datacard(skip_systematics=skip_systematics)

    shutil.copyfile("Datacard_tutorial.txt", f"Datacard_tutorial_{cat}.txt")
    shutil.copyfile("Datacard_tutorial.pkl", f"Datacard_tutorial_{cat}.pkl")

    # makeDatacard consumes all yields in yields_tutorial/, so clean after each single-cat card
    rm_glob("yields_tutorial/*.pkl")


def run_combined_datacard(
    cats: Iterable[str], channel: str, ws_path: str, skip_systematics: bool = False
) -> None:
    cats_list = list(cats)
    cats_arg = ",".join(cats_list)  # matches: IFS=, ; echo "${CATS[*]}"

    print(
        f"---------------------------- {channel}: Combined ----------------------------"
    )
    print(f"========== Cats: {cats_arg} =============")
    print(ws_path)

    run_yields(ws_path=ws_path, cats=cats_arg)
    make_datacard(skip_systematics=skip_systematics)

    shutil.copyfile("Datacard_tutorial.txt", f"Datacard_tutorial_{channel}.txt")
    shutil.copyfile("Datacard_tutorial.pkl", f"Datacard_tutorial_{channel}.pkl")
    # intentionally no cleanup here (you reuse yields later)


def main() -> None:
    root_io = os.environ.get("ROOT_IO")
    if not root_io:
        raise SystemExit("ERROR: ROOT_IO environment variable is not set.")

    ensure_dir(Path("yields_tutorial"))

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--nosys",
        action="store_true",
        help="Disable systematic and MC statistical uncertainties",
    )
    args = parser.parse_args()
    skip_systematics = args.nosys

    # --- ggH ---
    ggh_cats = get_categories("ggH")
    ggh_path = f"{root_io}/tuples/split_tuples/ggH/ws_GG2H"
    ggh_path = f"../inputs/ws_signal/"

    vbf_cats = get_categories("VBF")
    vbf_path = f"{root_io}/tuples/split_tuples/VBF/ws_VBF"
    vbf_path = f"../inputs/ws_signal/"

    print("---------------------------- ggH -----------------------------------")
    for cat in ggh_cats:
        run_datacard(
            cat=cat, channel="ggH", ws_path=ggh_path, skip_systematics=skip_systematics
        )

    run_combined_datacard(
        cats=ggh_cats,
        channel="ggH",
        ws_path=ggh_path,
        skip_systematics=skip_systematics,
    )

    # Save the ggH yields for the final Datacard
    ggh_dir = Path("yields_tutorial/ggH")
    ensure_dir(ggh_dir)

    for cat in ggh_cats:
        src = Path(f"yields_tutorial/{cat}.pkl")
        dst = ggh_dir / f"{cat}.pkl"
        if src.exists():
            print(f"mv -v {src} {dst}")
            shutil.move(str(src), str(dst))
        else:
            print(f"WARNING: expected yield file not found: {src}")

    rm_glob("yields_tutorial/*.pkl")

    print("---------------------------- VBF -----------------------------------")
    for cat in vbf_cats:
        run_datacard(
            cat=cat, channel="VBF", ws_path=vbf_path, skip_systematics=skip_systematics
        )

    run_combined_datacard(
        cats=vbf_cats,
        channel="VBF",
        ws_path=vbf_path,
        skip_systematics=skip_systematics,
    )

    # --- Final Combined ---
    print("---------------------------- Combined -----------------------------------")

    # Move back ggH cats to yields_tutorial/
    for cat in ggh_cats:
        src = ggh_dir / f"{cat}.pkl"
        dst = Path(f"yields_tutorial/{cat}.pkl")
        if src.exists():
            print(f"mv -v {src} {dst}")
            shutil.move(str(src), str(dst))
        else:
            print(f"WARNING: expected stashed ggH yield not found: {src}")

    make_datacard(skip_systematics=skip_systematics)
    shutil.copyfile("Datacard_tutorial.txt", f"Datacard_tutorial_All.txt")
    shutil.copyfile("Datacard_tutorial.pkl", f"Datacard_tutorial_All.pkl")

    # Final cleanup
    rm_glob("yields_tutorial/*.pkl")


if __name__ == "__main__":
    main()
