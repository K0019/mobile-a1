import argparse
import os
import shutil
import subprocess
import sys


def _read_manifest_lines(manifest_path: str) -> list[str]:
    with open(manifest_path, "r", encoding="utf-8") as f:
        lines = []
        for raw in f:
            line = raw.strip()
            if not line:
                continue
            lines.append(line)
        return lines


def _find_case_insensitive(path: str) -> str | None:
    """Find a file on a case-insensitive basis (Windows-hosted repo can still contain mixed-case names).

    Returns an existing path if found, otherwise None.
    """
    if os.path.exists(path):
        return path

    parent = os.path.dirname(path)
    target = os.path.basename(path).lower()

    if not os.path.isdir(parent):
        return None

    try:
        for name in os.listdir(parent):
            if name.lower() == target:
                candidate = os.path.join(parent, name)
                if os.path.exists(candidate):
                    return candidate
    except OSError:
        return None

    return None


def _iter_compiled_ktx2_paths_from_manifest(lines: list[str]) -> list[str]:
    out: list[str] = []
    for p in lines:
        # Manifest uses forward slashes. Check case-insensitively.
        p_lower = p.lower()
        if not p_lower.startswith("compiledassets/"):
            continue
        if not p_lower.endswith(".ktx2"):
            continue
        # Skip already-platform-specific outputs.
        if p_lower.startswith("compiledassets/android/"):
            continue
        out.append(p)
    return out


def _iter_raw_image_paths_from_manifest(lines: list[str]) -> list[str]:
    """Find raw image files (PNG, JPG) in images/ folder that need compilation."""
    out: list[str] = []
    image_extensions = (".png", ".jpg", ".jpeg")
    for p in lines:
        p_lower = p.lower()
        if not p_lower.startswith("images/"):
            continue
        if not any(p_lower.endswith(ext) for ext in image_extensions):
            continue
        out.append(p)
    return out


def _iter_compiled_paths_from_manifest(lines: list[str]) -> list[str]:
    out: list[str] = []
    for p in lines:
        # Check case-insensitively.
        p_lower = p.lower()
        if not p_lower.startswith("compiledassets/"):
            continue
        # Skip already-platform-specific outputs.
        if p_lower.startswith("compiledassets/android/"):
            continue
        out.append(p)
    return out


def _run_assetcompiler(assetcompiler: str, assets_root: str, input_ktx2: str, output_dir: str, dry_run: bool) -> int:
    cmd = [
        assetcompiler,
        "--root",
        assets_root,
        "--input",
        input_ktx2,
        "--output",
        output_dir,
        "--platform",
        "android",
        "--format",
        "ASTC",
        "--recompress-ktx2",
    ]

    if dry_run:
        print("DRY RUN:", " ".join([f'"{c}"' if " " in c else c for c in cmd]))
        return 0

    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if proc.returncode != 0:
        sys.stdout.write(proc.stdout)
    return proc.returncode


def _run_assetcompiler_compile_image(assetcompiler: str, assets_root: str, input_image: str, output_dir: str, dry_run: bool) -> int:
    """Compile a raw image (PNG, JPG) to ASTC KTX2 format."""
    cmd = [
        assetcompiler,
        "--root",
        assets_root,
        "--input",
        input_image,
        "--output",
        output_dir,
        "--platform",
        "android",
        "--format",
        "ASTC",
    ]

    if dry_run:
        print("DRY RUN:", " ".join([f'"{c}"' if " " in c else c for c in cmd]))
        return 0

    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if proc.returncode != 0:
        sys.stdout.write(proc.stdout)
    return proc.returncode


def _quote_cmd(cmd: list[str]) -> str:
    parts = []
    for c in cmd:
        if " " in c or "\t" in c:
            parts.append(f'"{c}"')
        else:
            parts.append(c)
    return " ".join(parts)


def main() -> int:
    parser = argparse.ArgumentParser(description="Rebuild Android ASTC texture variants from compiled KTX2 assets")
    parser.add_argument("--assetcompiler", required=True, help="Path to AssetCompiler executable")
    parser.add_argument("--assets-root", default="Assets", help="Path to Assets root directory")
    parser.add_argument("--manifest", default=None, help="Path to asset_manifest.txt (defaults to <assets-root>/asset_manifest.txt)")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--limit", type=int, default=0, help="Max number of textures to process (0 = no limit)")
    parser.add_argument(
        "--include-non-textures",
        action="store_true",
        help="Also copy non-texture compiledassets (materials/meshes/etc) into compiledassets/android",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Force recompression even if output is newer than input (skip timestamp check)",
    )
    args = parser.parse_args()

    assets_root = os.path.abspath(args.assets_root)
    manifest_path = os.path.abspath(args.manifest) if args.manifest else os.path.join(assets_root, "asset_manifest.txt")

    # Convert to absolute path - subprocess.run on Windows requires this for forward-slash paths
    assetcompiler = os.path.abspath(args.assetcompiler)
    if not os.path.isfile(assetcompiler):
        print(f"ERROR: AssetCompiler not found: {assetcompiler}")
        return 2

    if not os.path.isfile(manifest_path):
        print(f"ERROR: asset manifest not found: {manifest_path}")
        return 2

    lines = _read_manifest_lines(manifest_path)
    if args.include_non_textures:
        work_list = _iter_compiled_paths_from_manifest(lines)
    else:
        work_list = _iter_compiled_ktx2_paths_from_manifest(lines)

    if args.limit and args.limit > 0:
        work_list = work_list[: args.limit]

    failures = 0
    processed = 0
    skipped = 0

    for rel in work_list:
        # rel is like: compiledassets/textures/image_0.ktx2
        in_path = os.path.join(assets_root, *rel.split("/"))
        in_path = _find_case_insensitive(in_path)
        if not in_path:
            print(f"WARN: missing input KTX2: {rel}")
            failures += 1
            continue

        # Strip the "compiledassets/" prefix (case-insensitive)
        rel_after_prefix = rel[len("compiledassets/") :] if rel.lower().startswith("compiledassets/") else rel
        out_dir = os.path.join(assets_root, "compiledassets", "android", *os.path.dirname(rel_after_prefix).split("/"))
        os.makedirs(out_dir, exist_ok=True)

        # Ensure output filename matches manifest path (usually lowercase) to avoid Android case-sensitivity issues.
        desired_out_path = os.path.join(assets_root, "compiledassets", "android", *rel_after_prefix.split("/"))

        # Timestamp-based caching: skip if output exists and is newer than input
        if not args.force and os.path.isfile(desired_out_path):
            in_mtime = os.path.getmtime(in_path)
            out_mtime = os.path.getmtime(desired_out_path)
            if out_mtime >= in_mtime:
                skipped += 1
                continue

        if rel.lower().endswith(".ktx2"):
            try:
                # Recompress texture to ASTC.
                rc = _run_assetcompiler(assetcompiler, assets_root, in_path, out_dir, args.dry_run)
                processed += 1
                if rc != 0:
                    print(f"ERROR: AssetCompiler failed for {rel} (exit={rc})")
                    failures += 1
                    continue

                if args.dry_run:
                    continue

                # AssetCompiler writes using the input stem (may have different case). Rename into canonical path.
                original_produced_path = os.path.join(out_dir, os.path.basename(in_path))
                original_produced_path = _find_case_insensitive(original_produced_path)
                if not original_produced_path or not os.path.isfile(original_produced_path):
                    print(f"ERROR: could not locate produced output for {rel} in {out_dir}")
                    failures += 1
                    continue

                original_produced_meta = _find_case_insensitive(original_produced_path + ".meta")

                if os.path.abspath(original_produced_path) != os.path.abspath(desired_out_path):
                    os.makedirs(os.path.dirname(desired_out_path), exist_ok=True)
                    if os.path.exists(desired_out_path):
                        os.remove(desired_out_path)
                    os.replace(original_produced_path, desired_out_path)

                desired_meta = desired_out_path + ".meta"

                # If meta already exists at the destination name, leave it.
                if os.path.isfile(desired_meta):
                    continue

                if original_produced_meta and os.path.isfile(original_produced_meta):
                    if os.path.abspath(original_produced_meta) == os.path.abspath(desired_meta):
                        continue
                    os.makedirs(os.path.dirname(desired_meta), exist_ok=True)
                    if os.path.exists(desired_meta):
                        os.remove(desired_meta)
                    os.replace(original_produced_meta, desired_meta)
            except Exception as e:
                print(f"ERROR: exception processing {rel}: {e}")
                failures += 1
                continue

        else:
            # Copy non-texture compiled asset as-is into the android platform folder.
            processed += 1
            if args.dry_run:
                print("DRY RUN: copy", in_path, "->", desired_out_path)
                continue

            os.makedirs(os.path.dirname(desired_out_path), exist_ok=True)
            shutil.copy2(in_path, desired_out_path)

            in_meta = in_path + ".meta"
            out_meta = desired_out_path + ".meta"
            in_meta = _find_case_insensitive(in_meta)
            if in_meta and os.path.isfile(in_meta):
                shutil.copy2(in_meta, out_meta)

    # -------------------------------------------------------------------------
    # Phase 2: Compile raw images from images/ folder to ASTC KTX2
    # -------------------------------------------------------------------------
    raw_images = _iter_raw_image_paths_from_manifest(lines)
    if args.limit and args.limit > 0:
        remaining = max(0, args.limit - processed)
        raw_images = raw_images[:remaining]

    for rel in raw_images:
        # rel is like: images/skybox.png
        in_path = os.path.join(assets_root, *rel.split("/"))
        in_path = _find_case_insensitive(in_path)
        if not in_path:
            print(f"WARN: missing input image: {rel}")
            failures += 1
            continue

        # Output goes to compiledassets/android/images/ with .ktx2 extension
        # e.g., images/skybox.png -> compiledassets/android/images/skybox.ktx2
        base_name = os.path.splitext(os.path.basename(rel))[0]
        rel_dir = os.path.dirname(rel)  # "images" or "images/subfolder"
        out_dir = os.path.join(assets_root, "compiledassets", "android", *rel_dir.split("/"))
        os.makedirs(out_dir, exist_ok=True)

        desired_out_path = os.path.join(out_dir, base_name + ".ktx2")

        # Timestamp-based caching: skip if output exists and is newer than input
        if not args.force and os.path.isfile(desired_out_path):
            in_mtime = os.path.getmtime(in_path)
            out_mtime = os.path.getmtime(desired_out_path)
            if out_mtime >= in_mtime:
                skipped += 1
                continue

        try:
            rc = _run_assetcompiler_compile_image(assetcompiler, assets_root, in_path, out_dir, args.dry_run)
            processed += 1
            if rc != 0:
                print(f"ERROR: AssetCompiler failed for {rel} (exit={rc})")
                failures += 1
                continue

            if args.dry_run:
                continue

            # AssetCompiler writes using input stem. Locate and rename if needed.
            original_produced_path = os.path.join(out_dir, base_name + ".ktx2")
            original_produced_path = _find_case_insensitive(original_produced_path)
            if not original_produced_path or not os.path.isfile(original_produced_path):
                print(f"ERROR: could not locate produced output for {rel} in {out_dir}")
                failures += 1
                continue

            # Rename to canonical lowercase path if needed
            if os.path.abspath(original_produced_path) != os.path.abspath(desired_out_path):
                if os.path.exists(desired_out_path):
                    os.remove(desired_out_path)
                os.replace(original_produced_path, desired_out_path)

            # Handle .meta file
            original_produced_meta = _find_case_insensitive(original_produced_path + ".meta")
            desired_meta = desired_out_path + ".meta"
            if original_produced_meta and os.path.isfile(original_produced_meta):
                if os.path.abspath(original_produced_meta) != os.path.abspath(desired_meta):
                    if os.path.exists(desired_meta):
                        os.remove(desired_meta)
                    os.replace(original_produced_meta, desired_meta)

        except Exception as e:
            print(f"ERROR: exception processing {rel}: {e}")
            failures += 1
            continue

    print(f"Done. processed={processed} skipped={skipped} failures={failures}")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
