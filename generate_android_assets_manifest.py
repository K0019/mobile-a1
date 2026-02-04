import argparse
import os
import sys

# Generates a case-preserving manifest for Android APK asset lookup.
#
# Android APK assets are case-sensitive. Our engine often requests asset paths in
# mixed/lowercase. The runtime VFS builds a lookup table using:
#   lowercase(requested_path) -> actual_on_disk_path
#
# This script must write the actual on-disk path (case-preserving) for each file.

_SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ASSETS_ROOT = os.path.join(_SCRIPT_DIR, "Assets")
OUTPUT_FILE = os.path.join(ASSETS_ROOT, "asset_manifest.txt")

# Windows reserved device names — os.path.relpath() fails on these.
_WIN_RESERVED = frozenset({
    "con", "prn", "aux", "nul",
    *(f"com{i}" for i in range(1, 10)),
    *(f"lpt{i}" for i in range(1, 10)),
})


def _collect_asset_paths(assets_root: str) -> list[str]:
    """Walk assets directory and collect all relative paths."""
    paths: list[str] = []
    for root, _dirs, files in os.walk(assets_root):
        files.sort()
        for name in files:
            if name == "asset_manifest.txt":
                continue
            # Skip Windows reserved device names (e.g. "nul") which break os.path.relpath.
            stem = name.split(".")[0].lower()
            if stem in _WIN_RESERVED:
                continue
            full_path = os.path.join(root, name)
            rel = os.path.relpath(full_path, assets_root).replace("\\", "/")
            paths.append(rel)
    return paths


def _read_existing_manifest(path: str) -> list[str] | None:
    """Read existing manifest, return None if doesn't exist."""
    if not os.path.isfile(path):
        return None
    try:
        with open(path, "r", encoding="utf-8") as f:
            return [line.rstrip("\n") for line in f if line.strip()]
    except Exception:
        return None


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate Android asset manifest")
    parser.add_argument(
        "--force",
        action="store_true",
        help="Force regeneration even if manifest is up-to-date",
    )
    args = parser.parse_args()

    if not os.path.isdir(ASSETS_ROOT):
        print(f"Error: Assets root '{ASSETS_ROOT}' does not exist.")
        return 1

    try:
        # Collect current asset paths
        current_paths = _collect_asset_paths(ASSETS_ROOT)
        file_count = len(current_paths)

        # Check if manifest needs updating
        if not args.force:
            existing_paths = _read_existing_manifest(OUTPUT_FILE)
            if existing_paths is not None and existing_paths == current_paths:
                print(f"Manifest unchanged ({file_count} files). Skipped write.")
                return 0

        # Write new manifest
        with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
            for rel in current_paths:
                f.write(rel + "\n")

        print(f"Generated asset manifest at: {OUTPUT_FILE}")
        print(f"Total files indexed: {file_count}")
        return 0
    except Exception as e:
        print(f"Error writing manifest: {e}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
