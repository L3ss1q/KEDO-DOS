#!/usr/bin/env python3
"""
Build a 1.44 MB KEDO BIOS floppy image.

Disk layout (standard 1.44 MB floppy: 80 cylinders, 2 heads, 18 sectors):
  LBA 0       : 512-byte boot sector
  LBA 1-70    : kernel load area (bootloader reads all 70 sectors)
  LBA 71      : reserved gap
  LBA 72-81   : 10-sector KEDO filesystem snapshot

Run from the project directory:
    python make_image.py

Optional:
    python make_image.py --boot path/to/boot.asm
    python make_image.py --kernel path/to/kernel.asm
    python make_image.py --output kedo-bios.img
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

SECTOR_SIZE = 512
FLOPPY_SECTORS = 2880
KERNEL_START_LBA = 1
KERNEL_SECTORS = 70
SNAPSHOT_START_LBA = 72
SNAPSHOT_SECTORS = 10
IMAGE_SIZE = SECTOR_SIZE * FLOPPY_SECTORS

BOOT_CANDIDATES = (
    "kedo_boot.asm",
    "boot/boot.asm",
    "boot.asm",
    "b.asm",
)
KERNEL_CANDIDATES = (
    "kedo_kernel.asm",
    "kernel/kernel.asm",
    "kernel.asm",
    "k.asm",
)
BOOT_BIN_CANDIDATES = ("boot.bin", "build/boot.bin")
KERNEL_BIN_CANDIDATES = ("kernel.bin", "build/kernel.bin")


def find_source(explicit: str | None, candidates: tuple[str, ...], label: str) -> Path:
    if explicit:
        path = Path(explicit).expanduser()
        if not path.is_file():
            raise SystemExit(f"{label} source not found: {path}")
        return path

    for candidate in candidates:
        path = Path(candidate)
        if path.is_file():
            return path

    choices = "\n".join(f"  - {name}" for name in candidates)
    raise SystemExit(
        f"Could not find the {label} NASM source.\n"
        f"Pass --{label} PATH or place it at one of:\n{choices}"
    )


def find_nasm(requested: str | None) -> str:
    """Find NASM through PATH, an explicit path, or common Windows installs."""
    if requested:
        candidate = Path(requested).expanduser()
        if candidate.is_file():
            return str(candidate)
        located = shutil.which(requested)
        if located:
            return located
        raise SystemExit(
            f"NASM executable not found: {requested}\\n"
            "Use --nasm with the full path to nasm.exe."
        )

    env_path = os.environ.get("NASM_PATH")
    if env_path:
        candidate = Path(env_path).expanduser()
        if candidate.is_file():
            return str(candidate)

    for command in ("nasm", "nasm.exe"):
        located = shutil.which(command)
        if located:
            return located

    roots = [
        Path(__file__).resolve().parent,
        Path.cwd(),
        Path(r"C:\\NASM"),
        Path(r"C:\\Program Files\\NASM"),
        Path(r"C:\\Program Files (x86)\\NASM"),
        Path(r"C:\\tools\\nasm"),
        Path(r"C:\\msys64\\usr\\bin"),
        Path(r"C:\\msys64\\ucrt64\\bin"),
        Path(r"C:\\msys64\\mingw64\\bin"),
        Path(r"C:\\msys64\\clang64\\bin"),
    ]
    for root in roots:
        for name in ("nasm.exe", "nasm"):
            candidate = root / name
            if candidate.is_file():
                return str(candidate)

    raise SystemExit(
        "I couldn't find NASM. The image builder needs NASM to turn the .asm "
        "files into bootable binary files.\\n\\n"
        "If NASM is installed, pass its exact path, for example:\\n"
        '  python make_image.py --nasm "C:\\\\NASM\\\\nasm.exe"\\n'
        "Or add the folder containing nasm.exe to your Windows PATH, then "
        "open a new terminal.\\n"
        "You can also set NASM_PATH to the full path of nasm.exe."
    )


def assemble(nasm: str, source: Path, output: Path, label: str) -> None:
    command = [nasm, "-f", "bin", str(source), "-o", str(output)]
    print(f"Assembling {label}: {source}")
    try:
        result = subprocess.run(command, check=False)
    except FileNotFoundError:
        raise SystemExit(
            "NASM was not found. Install NASM and make sure `nasm` is on PATH."
        )
    if result.returncode != 0:
        raise SystemExit(f"{label} assembly failed (NASM exit code {result.returncode}).")


def main() -> int:
    parser = argparse.ArgumentParser(description="Build the KEDO BIOS floppy image.")
    parser.add_argument("--boot", help="path to the bootloader .asm source")
    parser.add_argument("--kernel", help="path to the kernel .asm source")
    parser.add_argument("--output", default="kedo-bios.img", help="output image path")
    parser.add_argument("--nasm", help="NASM executable or full path to nasm.exe")
    parser.add_argument(
        "--rebuild",
        action="store_true",
        help="reassemble the .asm sources instead of using existing .bin files",
    )
    args = parser.parse_args()

    def first_existing(candidates: tuple[str, ...]) -> Path | None:
        for candidate in candidates:
            path = Path(candidate)
            if path.is_file():
                return path
        return None

    boot_bin = first_existing(BOOT_BIN_CANDIDATES)
    kernel_bin = first_existing(KERNEL_BIN_CANDIDATES)

    if not args.rebuild and boot_bin and kernel_bin:
        print(f"Using existing boot binary: {boot_bin}")
        print(f"Using existing kernel binary: {kernel_bin}")
        print("NASM is not needed for this build.")
    else:
        boot_source = find_source(args.boot, BOOT_CANDIDATES, "boot")
        kernel_source = find_source(args.kernel, KERNEL_CANDIDATES, "kernel")
        nasm = find_nasm(args.nasm)

        build_dir = Path("build")
        build_dir.mkdir(exist_ok=True)
        boot_bin = build_dir / "boot.bin"
        kernel_bin = build_dir / "kernel.bin"

        assemble(nasm, boot_source, boot_bin, "bootloader")
        assemble(nasm, kernel_source, kernel_bin, "kernel")

    boot_data = boot_bin.read_bytes()
    kernel_data = kernel_bin.read_bytes()

    if len(boot_data) != SECTOR_SIZE:
        raise SystemExit(
            f"Boot sector must be exactly 512 bytes; got {len(boot_data)} bytes."
        )
    if boot_data[510:512] != b"\x55\xAA":
        raise SystemExit("Boot sector is missing the 0x55AA boot signature.")

    max_kernel_bytes = KERNEL_SECTORS * SECTOR_SIZE
    if len(kernel_data) > max_kernel_bytes:
        raise SystemExit(
            f"Kernel too large: {len(kernel_data)} bytes; "
            f"maximum for this bootloader is {max_kernel_bytes} bytes "
            f"({KERNEL_SECTORS} sectors)."
        )

    kernel_end_lba = KERNEL_START_LBA + KERNEL_SECTORS
    if kernel_end_lba > SNAPSHOT_START_LBA:
        raise SystemExit("Configured kernel load area overlaps the filesystem snapshot.")

    image = bytearray(IMAGE_SIZE)
    image[0:SECTOR_SIZE] = boot_data

    kernel_offset = KERNEL_START_LBA * SECTOR_SIZE
    image[kernel_offset:kernel_offset + len(kernel_data)] = kernel_data

    # Leave the snapshot area zeroed on a fresh image. The kernel recognizes
    # the invalid/empty snapshot and formats it on first boot.
    snapshot_offset = SNAPSHOT_START_LBA * SECTOR_SIZE
    snapshot_end = snapshot_offset + SNAPSHOT_SECTORS * SECTOR_SIZE
    image[snapshot_offset:snapshot_end] = bytes(SNAPSHOT_SECTORS * SECTOR_SIZE)

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(image)

    print()
    print(f"Created: {output_path}")
    print(f"Image size: {len(image):,} bytes (1.44 MB floppy)")
    print(f"Boot sector: LBA 0")
    print(
        f"Kernel: {len(kernel_data):,} bytes at LBA {KERNEL_START_LBA}; "
        f"limit {max_kernel_bytes:,} bytes"
    )
    print(
        f"Filesystem snapshot: LBA {SNAPSHOT_START_LBA}-"
        f"{SNAPSHOT_START_LBA + SNAPSHOT_SECTORS - 1}"
    )
    print()
    print("Note: this build creates a blank filesystem snapshot.")
    print("Your existing saved KEDO files remain in the old image, not this new one.")
    print("If you edited the .asm files and want fresh binaries, run with --rebuild.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
