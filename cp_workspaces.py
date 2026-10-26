import os
import shutil
from pathlib import Path


def ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def copy_files(files, output_dir: Path) -> None:
    ensure_dir(output_dir)

    for src in files:
        src = Path(src)

        if not src.exists():
            print(f"WARNING: file does not exist: {src}")
            continue

        dst = output_dir / src.name

        print(f"cp {src} {dst}")
        shutil.copy2(src, dst)


def main() -> None:
    root_io = os.environ.get("ROOT_IO")

    if not root_io:
        raise SystemExit("ERROR: ROOT_IO environment variable is not set.")

    root_io = Path(root_io)

    signal_workspaces = [
        root_io
        / "tuples/split_tuples/ggH/ws_GG2H/output_signal_M125_13TeV_cats_pythia8_GG2H.root",
        root_io
        / "tuples/split_tuples/VBF/ws_VBF/output_signal_M125_13TeV_cats_pythia8_VBF.root",
    ]

    data_workspaces = [
        root_io / "tuples/split_tuples/ws/allData_combined.root",
    ]

    copy_files(signal_workspaces, Path("./inputs/ws_signal"))
    copy_files(data_workspaces, Path("./inputs/ws"))


if __name__ == "__main__":
    main()
