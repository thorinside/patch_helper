#!/usr/bin/env python3
"""Build a fresh development ZIP with paths relative to the NT's SD root."""
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile

root = Path(__file__).resolve().parents[1]
files = {
    "programs/plug-ins/patch_helper.o": root / "plugins/patch_helper.o",
    "programs/helper/ThPh.lua": root / "helper/ThPh.lua",
    "README.md": root / "README.md",
}
# Check inputs first so a missing companion cannot replace a usable package.
for source in files.values():
    if not source.is_file() or source.stat().st_size == 0:
        raise SystemExit(f"Missing or empty package input: {source}")

output = root / "build/patch_helper-preview.zip"
output.parent.mkdir(parents=True, exist_ok=True)
with ZipFile(output, "w", compression=ZIP_DEFLATED) as archive:
    for destination, source in files.items():
        archive.write(source, destination)

with ZipFile(output) as archive:
    assert set(archive.namelist()) == set(files)
    for destination, source in files.items():
        assert archive.read(destination) == source.read_bytes(), destination
print(output)
