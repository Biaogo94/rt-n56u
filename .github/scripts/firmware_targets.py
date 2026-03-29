#!/usr/bin/env python3

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Optional


REPO_ROOT = Path(__file__).resolve().parents[2]
TEMPLATES_DIR = REPO_ROOT / "trunk" / "configs" / "templates"
BOARDS_DIR = REPO_ROOT / "trunk" / "configs" / "boards"
CATALOG_PATH = REPO_ROOT / ".github" / "firmware-targets.json"
WORKFLOW_PATH = REPO_ROOT / ".github" / "workflows" / "build.yml"

VARIANT_ORDER = ["mt7620", "mt7628", "mt7621", "mt7621-usb"]

CI_GROUPS = {
    "mt7620": ["PSG1208", "PSG1218", "NEWIFI-MINI", "MI-MINI"],
    "mt7628": ["HC5861B", "MI-NANO", "MZ-R13", "MZ-R13P", "360P2"],
    "mt7621": ["K2P_nano", "K2P", "DIR-878", "RM2100", "CR660x"],
    "mt7621-usb": ["XY-C1", "JCG-836PRO", "JCG-AC860M", "MI-R3G", "NEWIFI3", "B70"],
}

PRIMARY_TIER_TARGETS = {
    "CR660x",
    "DIR-878",
    "K2P",
    "K2P_nano",
    "RM2100",
}

SECONDARY_TIER_TARGETS = {
    "DIR-882",
    "JCG-836PRO",
    "JCG-AC860M",
    "JCG-Q20",
    "JCG-Y2",
    "K2P-USB",
    "MI-R3P",
    "MI-R3P-breed",
    "MR2600",
    "NETGEAR-BZV",
    "NETGEAR-CHJ",
    "R2100",
    "WDR7300",
    "XY-C1",
}


def parse_config(path: Path) -> dict:
    data = {}
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("###"):
            continue
        match = re.match(r"^(CONFIG_[A-Z0-9_]+)=(.*)$", line)
        if not match:
            continue
        key, value = match.groups()
        if value.startswith('"') and value.endswith('"'):
            value = value[1:-1]
        data[key] = value
    return data


def get_radio_family(board_config: dict) -> str:
    radios = []
    for key in ("CONFIG_RT_FIRST_CARD", "CONFIG_RT_SECOND_CARD"):
        value = board_config.get(key, "0")
        if value and value != "0":
            radios.append(value)
    return "+".join(radios) if radios else "none"


def get_ci_group(target: str) -> str:
    for group_name in VARIANT_ORDER:
        if target in CI_GROUPS[group_name]:
            return group_name
    return ""


def get_release_tier(target: str, soc: str, radios: str) -> str:
    if target in PRIMARY_TIER_TARGETS:
        return "primary"
    if target in SECONDARY_TIER_TARGETS:
        return "secondary"
    if soc == "mt7621" and ("7615" in radios or "7915" in radios):
        return "secondary"
    return "compatibility"


def build_catalog() -> dict:
    targets = []
    template_paths = sorted(TEMPLATES_DIR.glob("*.config"))

    for template_path in template_paths:
        template_config = parse_config(template_path)
        target = template_path.stem
        board_id = template_config["CONFIG_FIRMWARE_PRODUCT_ID"]
        soc = template_config["CONFIG_PRODUCT"].lower()
        kernel_config = template_config.get("CONFIG_FIRMWARE_KERNEL_CONFIG", "kernel-3.4.x.config")
        usb_enabled = template_config.get("CONFIG_FIRMWARE_ENABLE_USB", "n") == "y"

        board_config_path = BOARDS_DIR / board_id / kernel_config
        if not board_config_path.exists():
            raise FileNotFoundError(f"Board config not found for {target}: {board_config_path}")

        board_config = parse_config(board_config_path)
        radios = get_radio_family(board_config)
        ci_group = get_ci_group(target)
        release_tier = get_release_tier(target, soc, radios)

        targets.append(
            {
                "target": target,
                "board_id": board_id,
                "soc": soc,
                "radios": radios,
                "usb": usb_enabled,
                "kernel_config": kernel_config,
                "ci_group": ci_group,
                "release_tier": release_tier,
            }
        )

    return {
        "schema_version": 1,
        "variants": VARIANT_ORDER,
        "groups": CI_GROUPS,
        "targets": targets,
    }


def load_catalog_targets(catalog_path: Path) -> dict:
    catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
    return {item["target"]: item for item in catalog["targets"]}


def get_target_from_image_name(image_path: Path, kernel_version: str) -> Optional[str]:
    prefix = "Padavan-"
    suffix = f"-{kernel_version}"
    stem = image_path.stem

    if not stem.startswith(prefix):
        return None
    if not stem.endswith(suffix):
        return None

    target = stem[len(prefix):-len(suffix)]
    return target or None


def write_manifest(
    catalog_path: Path,
    images_dir: Path,
    build_mode: str,
    build_variant: str,
    kernel_version: str,
    git_revision: str,
) -> None:
    targets = load_catalog_targets(catalog_path)
    images = []

    for image_path in sorted(images_dir.glob("Padavan-*.trx")):
        target = get_target_from_image_name(image_path, kernel_version)
        if not target:
            continue
        if target not in targets:
            raise ValueError(f"Unknown firmware target in image name: {image_path.name}")

        meta = dict(targets[target])
        meta["filename"] = image_path.name
        meta["kernel_version"] = kernel_version
        meta["git_revision"] = git_revision
        meta["size_bytes"] = image_path.stat().st_size
        images.append(meta)

    manifest = {
        "build_mode": build_mode,
        "build_variant": build_variant,
        "kernel_version": kernel_version,
        "git_revision": git_revision,
        "images": images,
    }

    (images_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )


def yaml_list(items, indent: int) -> str:
    prefix = " " * indent
    return "\n".join(f"{prefix}- {item}" for item in items)


def render_build_workflow(catalog: dict) -> str:
    target_names = [item["target"] for item in catalog["targets"]]
    manual_target_options = yaml_list(target_names, 10)
    variant_options = yaml_list(catalog["variants"], 10)

    return f"""# This file is generated by .github/scripts/firmware_targets.py
# Do not edit it directly; update the target catalog generator instead.
name: Build Firmware

concurrency:
  group: build-firmware-${{{{ github.workflow }}}}-${{{{ github.event_name }}}}-${{{{ github.event.pull_request.number || github.ref_name }}}}-${{{{ github.event.inputs.build_mode || 'auto' }}}}-${{{{ github.event.inputs.target || github.event.inputs.variant || 'default' }}}}
  cancel-in-progress: true

on:
  push:
    branches:
      - master
      - main
      - performance-optimization
    paths-ignore:
      - '**.md'
      - 'docs/**'
  pull_request:
    branches:
      - master
      - main
  workflow_dispatch:
    inputs:
      build_mode:
        description: 'Manual build mode'
        required: true
        default: 'single-target'
        type: choice
        options:
{yaml_list(['single-target', 'variant-batch'], 10)}
      target:
        description: 'Build target model'
        required: true
        default: 'PSG1218'
        type: choice
        options:
{manual_target_options}
      variant:
        description: 'Batch build variant'
        required: true
        default: 'mt7621'
        type: choice
        options:
{variant_options}

env:
  IMAGES_DIR: /opt/images

jobs:
  build:
    runs-on: ubuntu-22.04
    steps:
      - name: Checkout code
        uses: actions/checkout@v4

      - name: Verify generated files
        run: |
          python3 .github/scripts/firmware_targets.py check-sync

      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y --no-install-recommends \\
            build-essential gcc g++ make cmake autoconf automake \\
            libtool libtool-bin pkg-config flex bison gawk gperf \\
            zlib1g-dev libgmp3-dev libmpc-dev libmpfr-dev libncurses5-dev \\
            libltdl-dev python3 python3-docutils gettext autopoint texinfo \\
            help2man unzip xxd cpio wget curl git fakeroot kmod file \\
            ca-certificates

      - name: Restore toolchain cache
        id: cache-toolchain
        uses: actions/cache@v4
        with:
          path: toolchain-mipsel/toolchain-3.4.x
          key: ${{{{ runner.os }}}}-padavan-toolchain-3.4.x-${{{{ hashFiles('toolchain-mipsel/dl_toolchain.sh') }}}}

      - name: Download toolchain
        if: steps.cache-toolchain.outputs.cache-hit != 'true'
        run: |
          cd toolchain-mipsel
          sh dl_toolchain.sh
          cd ..

      - name: Verify toolchain
        run: |
          ls -la toolchain-mipsel/toolchain-3.4.x/bin/ | head -10
          toolchain-mipsel/toolchain-3.4.x/bin/mipsel-linux-uclibc-gcc --version || true

      - name: Build firmware
        id: build
        shell: bash
        run: |
          set -euo pipefail

          cd trunk
          mkdir -p "${{{{ env.IMAGES_DIR }}}}"
          shopt -s nullglob

          catalog_script="${{{{ github.workspace }}}}/.github/scripts/firmware_targets.py"
          kernel_ver="$(sed -n 's/^FIRMWARE_KERNEL_VER=\\"\\([^\\"]*\\)\\"/\\1/p' Makefile | head -n1)"
          [ -n "$kernel_ver" ] || kernel_ver="unknown"
          git_version="$(git rev-parse --short=7 HEAD 2>/dev/null || echo unknown)"

          set_output() {{
            local key="$1"
            local value="$2"
            echo "${{key}}=${{value}}" >> "$GITHUB_ENV"
            echo "${{key}}=${{value}}" >> "$GITHUB_OUTPUT"
          }}

          set_output "KERNEL_VER" "$kernel_ver"
          set_output "GIT_VERSION" "$git_version"
          set_output "kernel_ver" "$kernel_ver"
          set_output "git_version" "$git_version"

          collect_image() {{
            local model="$1"
            local image
            local dst

            for image in images/*.trx; do
              dst="${{{{ env.IMAGES_DIR }}}}/Padavan-${{model}}-${{kernel_ver}}.trx"
              cp -f "$image" "$dst"
              echo "Collected $image -> $dst"
            done
          }}

          if [ "${{{{ github.event_name }}}}" = "workflow_dispatch" ] && [ "${{{{ github.event.inputs.build_mode }}}}" = "single-target" ]; then
            target="${{{{ github.event.inputs.target }}}}"
            echo "Building single target: $target"
            fakeroot ./build_firmware_modify "$target"
            collect_image "$target"
            set_output "BUILD_MODE" "single-target"
            set_output "BUILD_VARIANT" ""
            set_output "build_mode" "single-target"
            set_output "build_variant" ""
            set_output "artifact_name" "Padavan-${{target}}-${{kernel_ver}}"
          else
            variant="${{{{ github.event.inputs.variant || 'mt7621' }}}}"
            targets="$(python3 "$catalog_script" list-group "$variant")"
            if [ -z "$targets" ]; then
              echo "No targets defined for variant: $variant" >&2
              exit 1
            fi

            echo "Building targets: $targets"
            for model in $targets; do
              echo "=== Building $model ==="
              fakeroot ./build_firmware_ci "$model"
              collect_image "$model"
              ./clear_tree_simple >/dev/null 2>&1 || true
            done

            set_output "BUILD_MODE" "variant-batch"
            set_output "BUILD_VARIANT" "${{variant}}"
            set_output "build_mode" "variant-batch"
            set_output "build_variant" "${{variant}}"
            set_output "artifact_name" "Padavan-${{variant}}-${{kernel_ver}}-${{git_version}}"
          fi

      - name: List built images
        run: |
          ls -lh "${{{{ env.IMAGES_DIR }}}}/" || echo "No images built"
          echo "=== Image sizes ==="
          du -h "${{{{ env.IMAGES_DIR }}}}"/*.trx 2>/dev/null || true

      - name: Generate checksums and manifest
        if: success()
        env:
          BUILD_MODE: ${{{{ steps.build.outputs.build_mode }}}}
          BUILD_VARIANT: ${{{{ steps.build.outputs.build_variant }}}}
          GIT_VERSION: ${{{{ steps.build.outputs.git_version }}}}
          KERNEL_VER: ${{{{ steps.build.outputs.kernel_ver }}}}
        run: |
          set -euo pipefail

          if ls "${{{{ env.IMAGES_DIR }}}}"/*.trx 1> /dev/null 2>&1; then
            cd "${{{{ env.IMAGES_DIR }}}}"
            md5sum *.trx | tee md5sum.txt

            python3 "${{{{ github.workspace }}}}/.github/scripts/firmware_targets.py" write-manifest \\
              --images-dir "${{{{ env.IMAGES_DIR }}}}" \\
              --build-mode "${{{{ env.BUILD_MODE }}}}" \\
              --build-variant "${{{{ env.BUILD_VARIANT }}}}" \\
              --kernel-version "${{{{ env.KERNEL_VER }}}}" \\
              --git-revision "${{{{ env.GIT_VERSION }}}}"
          else
            echo "No firmware images to archive"
          fi

      - name: Upload firmware artifact
        if: success()
        uses: actions/upload-artifact@v4
        with:
          name: ${{{{ steps.build.outputs.artifact_name || 'firmware' }}}}
          path: |
            ${{{{ env.IMAGES_DIR }}}}/*.trx
            ${{{{ env.IMAGES_DIR }}}}/md5sum.txt
            ${{{{ env.IMAGES_DIR }}}}/manifest.json
          retention-days: 30
"""


def write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8", newline="\n")


def command_dump_catalog(args: argparse.Namespace) -> int:
    content = json.dumps(build_catalog(), indent=2, ensure_ascii=False) + "\n"
    if args.output:
        write_text(Path(args.output), content)
    else:
        sys.stdout.write(content)
    return 0


def command_list_group(args: argparse.Namespace) -> int:
    catalog = build_catalog()
    sys.stdout.write(" ".join(catalog["groups"].get(args.group, [])))
    return 0


def command_write_manifest(args: argparse.Namespace) -> int:
    write_manifest(
        catalog_path=Path(args.catalog),
        images_dir=Path(args.images_dir),
        build_mode=args.build_mode,
        build_variant=args.build_variant,
        kernel_version=args.kernel_version,
        git_revision=args.git_revision,
    )
    return 0


def command_render_build_workflow(args: argparse.Namespace) -> int:
    content = render_build_workflow(build_catalog())
    if args.output:
        write_text(Path(args.output), content)
    else:
        sys.stdout.write(content)
    return 0


def command_check_sync(args: argparse.Namespace) -> int:
    expected_catalog = json.dumps(build_catalog(), indent=2, ensure_ascii=False) + "\n"
    expected_workflow = render_build_workflow(build_catalog())

    problems = []

    if not CATALOG_PATH.exists():
        problems.append(f"Missing generated catalog: {CATALOG_PATH}")
    elif CATALOG_PATH.read_text(encoding="utf-8") != expected_catalog:
        problems.append("Catalog file is out of sync with firmware_targets.py")

    if not WORKFLOW_PATH.exists():
        problems.append(f"Missing generated workflow: {WORKFLOW_PATH}")
    elif WORKFLOW_PATH.read_text(encoding="utf-8") != expected_workflow:
        problems.append("Build workflow is out of sync with firmware_targets.py")

    if problems:
        for problem in problems:
            print(problem, file=sys.stderr)
        return 1
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Manage firmware target metadata and generated workflow files.")
    subparsers = parser.add_subparsers(dest="command", required=True)

    dump_catalog = subparsers.add_parser("dump-catalog", help="Render the checked-in firmware target catalog.")
    dump_catalog.add_argument("--output", help="Write catalog JSON to this path.")
    dump_catalog.set_defaults(func=command_dump_catalog)

    list_group = subparsers.add_parser("list-group", help="Print targets for a CI group as a space-separated list.")
    list_group.add_argument("group", choices=VARIANT_ORDER)
    list_group.set_defaults(func=command_list_group)

    write_manifest_parser = subparsers.add_parser("write-manifest", help="Write manifest.json for built firmware images.")
    write_manifest_parser.add_argument("--images-dir", required=True, help="Directory containing Padavan-*.trx images.")
    write_manifest_parser.add_argument("--build-mode", required=True, help="Build mode stored in the manifest.")
    write_manifest_parser.add_argument("--build-variant", default="", help="Build variant stored in the manifest.")
    write_manifest_parser.add_argument("--kernel-version", required=True, help="Kernel version suffix embedded in image names.")
    write_manifest_parser.add_argument("--git-revision", required=True, help="Git revision stored in the manifest.")
    write_manifest_parser.add_argument("--catalog", default=str(CATALOG_PATH), help="Path to the firmware target catalog JSON.")
    write_manifest_parser.set_defaults(func=command_write_manifest)

    render_workflow = subparsers.add_parser("render-build-workflow", help="Render the Build Firmware workflow.")
    render_workflow.add_argument("--output", help="Write workflow YAML to this path.")
    render_workflow.set_defaults(func=command_render_build_workflow)

    check_sync = subparsers.add_parser("check-sync", help="Verify generated files match the current catalog renderer.")
    check_sync.set_defaults(func=command_check_sync)

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
