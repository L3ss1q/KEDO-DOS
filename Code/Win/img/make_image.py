from pathlib import Path

base = Path(__file__).resolve().parent
boot = (base / "boot.bin").read_bytes()
kernel = (base / "kernel.bin").read_bytes()

if len(boot) != 512:
    raise SystemExit(f"boot sector must be 512 bytes, got {len(boot)}")
if boot[510:512] != b"\x55\xaa":
    raise SystemExit("boot sector is missing the 0xAA55 signature")
if len(kernel) > 16 * 512:
    raise SystemExit(f"kernel too large: {len(kernel)} bytes; max is 8192")
image = bytearray(1_474_560)
image[:512] = boot
image[512:512 + len(kernel)] = kernel
(base / "kedo-bios.img").write_bytes(image)
print(f"Created {base / 'kedo-bios.img'} ({len(image)} bytes)")
print(f"Kernel: {len(kernel)} bytes / 8192-byte bootloader limit")
