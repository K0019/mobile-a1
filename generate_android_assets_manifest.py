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


def main() -> int:
    if not os.path.isdir(ASSETS_ROOT):
        print(f"Error: Assets root '{ASSETS_ROOT}' does not exist.")
        return 1

    try:
        file_count = 0
        with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
            for root, _dirs, files in os.walk(ASSETS_ROOT):
                files.sort()
                for name in files:
                    if name == "asset_manifest.txt":
                        continue
                    # Skip Windows reserved device names (e.g. "nul") which break os.path.relpath.
                    stem = name.split(".")[0].lower()
                    if stem in _WIN_RESERVED:
                        continue
                    full_path = os.path.join(root, name)
                    rel = os.path.relpath(full_path, ASSETS_ROOT).replace("\\", "/")
                    f.write(rel + "\n")
                    file_count += 1

        print(f"Generated asset manifest at: {OUTPUT_FILE}")
        print(f"Total files indexed: {file_count}")
        return 0
    except Exception as e:
        print(f"Error writing manifest: {e}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
