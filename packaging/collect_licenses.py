"""Collect redistribution notices into the application bundle."""

import importlib.metadata
import json
import shutil
import sys
import tempfile
from pathlib import Path

root = Path(sys.argv[1]).resolve()
output = Path(tempfile.mkdtemp(prefix="licenses-", dir=root / "build" / "windows"))
index = []


def copy(source: Path, package: str, name: str):
    destination = output / package / name
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, destination)


python_license = Path(sys.base_prefix) / "LICENSE.txt"
if not python_license.is_file():
    raise RuntimeError(f"Python license is missing: {python_license}")
copy(python_license, "Python", "LICENSE.txt")
index.append(f"Python {sys.version.split()[0]}")
for distribution in importlib.metadata.distributions():
    name = distribution.metadata.get("Name", "unknown").replace("/", "_").replace("\\", "_")
    found = False
    for item in distribution.files or []:
        if Path(str(item)).name.lower().startswith(("license", "copying", "notice")):
            source = Path(distribution.locate_file(item))
            if source.is_file():
                copy(source, name, Path(str(item)).name)
                found = True
    if found:
        index.append(f"{name} {distribution.version} (Python build environment)")
for folder in (Path(sys.base_prefix) / "tcl").glob("*/license*"):
    if folder.is_file():
        copy(folder, folder.parent.name, folder.name)
        index.append(folder.parent.name)
lock = json.loads((root / "frontend" / "package-lock.json").read_text(encoding="utf-8"))
for relative, entry in lock["packages"].items():
    if not relative.startswith("node_modules/") or entry.get("dev", False):
        continue
    source_dir = root / "frontend" / relative
    package = relative.removeprefix("node_modules/").replace("/", "_")
    for source in source_dir.iterdir():
        if source.is_file() and source.name.lower().startswith(("license", "copying", "notice")):
            copy(source, package, source.name)
    index.append(f"{package} {entry.get('version', '')} (frontend)")
(output / "README.txt").write_text("LogicLab bundled third-party notices\n\n" + "\n".join(sorted(index)) + "\n", encoding="utf-8")
print(output)
