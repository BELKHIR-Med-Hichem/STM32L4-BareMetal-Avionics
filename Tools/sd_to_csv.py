#!/usr/bin/env python3
"""
sd_to_csv.py  –  Convert bare-metal SD card log to CSV
=======================================================
Works in two modes:

  MODE A – from a binary dump file (recommended, works everywhere):
    python sd_to_csv.py sd_dump.bin output.csv

  MODE B – directly from the SD card device (Linux/macOS, requires sudo):
    sudo python sd_to_csv.py /dev/sdX output.csv --start-sector 100 --count 500

Dump-file creation:
  Linux:   sudo dd if=/dev/sdX bs=512 skip=100 count=500 of=sd_dump.bin
  Windows: see script comments below for PowerShell approach.

The script strips null-byte padding, skips empty sectors, and stops
automatically when it finds the first all-null sector (end of log).
"""

import sys
import os
import argparse
import struct

# ──────────────────────────────────────────────────────────────────────────────
#  Windows raw-disk helper
#  Works by opening \\.\PhysicalDriveN  with ctypes (requires Administrator).
# ──────────────────────────────────────────────────────────────────────────────
def read_windows_disk(device: str, start_sector: int, count: int) -> bytes:
    """Read `count` sectors from a Windows physical disk device."""
    import ctypes, ctypes.wintypes

    GENERIC_READ        = 0x80000000
    FILE_SHARE_READ     = 0x00000001
    FILE_SHARE_WRITE    = 0x00000002
    OPEN_EXISTING       = 3
    SECTOR_SIZE         = 512

    handle = ctypes.windll.kernel32.CreateFileW(
        device,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        None,
        OPEN_EXISTING,
        0,
        None
    )
    if handle == ctypes.wintypes.HANDLE(-1).value:
        raise OSError(f"Cannot open {device} – run as Administrator?")

    # Seek to start sector
    offset = start_sector * SECTOR_SIZE
    lo = offset & 0xFFFFFFFF
    hi = (offset >> 32) & 0xFFFFFFFF
    hi_c = ctypes.c_long(hi)
    ctypes.windll.kernel32.SetFilePointer(handle, lo, ctypes.byref(hi_c), 0)

    buf   = ctypes.create_string_buffer(SECTOR_SIZE * count)
    nread = ctypes.c_ulong(0)
    ctypes.windll.kernel32.ReadFile(handle, buf, len(buf), ctypes.byref(nread), None)
    ctypes.windll.kernel32.CloseHandle(handle)

    return bytes(buf[:nread.value])


# ──────────────────────────────────────────────────────────────────────────────
#  Core extractor
# ──────────────────────────────────────────────────────────────────────────────
def extract_text_from_sectors(raw: bytes) -> str:
    """
    Split `raw` into 512-byte sectors, strip null padding from each, and
    join all non-empty text.  Stop at the first fully-null sector.
    """
    SECTOR = 512
    total_sectors = len(raw) // SECTOR
    text_parts = []

    for i in range(total_sectors):
        sector = raw[i * SECTOR : (i + 1) * SECTOR]

        # All-null → end of log
        if all(b == 0 for b in sector):
            print(f"  ✓  End-of-log sentinel found at relative sector {i}.")
            break

        # Strip trailing nulls and decode as UTF-8 (ignore bad bytes)
        text = sector.rstrip(b'\x00').decode('utf-8', errors='replace')
        text_parts.append(text)

    return ''.join(text_parts)


# ──────────────────────────────────────────────────────────────────────────────
#  CSV validator / cleaner
# ──────────────────────────────────────────────────────────────────────────────
EXPECTED_HEADER = "pressure_hPa,temperature_C,pitch_deg,roll_deg"

def clean_csv(raw_text: str, output_path: str) -> int:
    """
    Write validated CSV rows to `output_path`.
    Returns the number of data rows written.
    """
    lines = raw_text.splitlines()
    rows_written = 0
    skipped      = 0

    with open(output_path, 'w', newline='') as f:
        # Always write the expected header first
        f.write(EXPECTED_HEADER + '\n')

        for line in lines:
            line = line.strip()
            if not line:
                continue

            # Skip header lines that may appear at sector boundaries
            if line.startswith("pressure") or line.startswith("CMD") \
               or line.startswith("ACMD")  or line.startswith("SD_"):
                continue

            # Validate: exactly 4 comma-separated numeric fields
            parts = line.split(',')
            if len(parts) != 4:
                skipped += 1
                continue
            try:
                float(parts[0])   # pressure
                float(parts[1])   # temperature
                float(parts[2])   # pitch
                float(parts[3])   # roll
            except ValueError:
                skipped += 1
                continue

            f.write(line + '\n')
            rows_written += 1

    if skipped:
        print(f"  ⚠  Skipped {skipped} malformed/non-data line(s).")
    return rows_written


# ──────────────────────────────────────────────────────────────────────────────
#  Entry point
# ──────────────────────────────────────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(
        description="Convert bare-metal SD card log to CSV."
    )
    parser.add_argument("source",
        help="Binary dump file (e.g. sd_dump.bin) OR raw device (e.g. /dev/sdX or \\\\.\\PhysicalDrive1).")
    parser.add_argument("output",
        help="Output CSV file (e.g. output.csv).")
    parser.add_argument("--start-sector", type=int, default=100,
        help="First data sector on the card (default: 100).")
    parser.add_argument("--count", type=int, default=2000,
        help="Maximum number of sectors to read when using a device (default: 2000 = 1 MB).")

    args = parser.parse_args()

    print(f"\n  Source  : {args.source}")
    print(f"  Output  : {args.output}")
    print(f"  Sectors : starting at {args.start_sector}, max {args.count}\n")

    # ── Load raw bytes ────────────────────────────────────────────────────────
    src = args.source

    if os.path.isfile(src):
        # Binary dump file – read all of it (sectors are already relative 0-based)
        print(f"  Reading dump file ({os.path.getsize(src):,} bytes)…")
        with open(src, 'rb') as f:
            raw = f.read()

    elif src.startswith('/dev/') or src.startswith('\\\\.\\'):
        # Raw device
        print(f"  Opening device {src} (may require elevated privileges)…")
        if sys.platform == 'win32':
            raw = read_windows_disk(src, args.start_sector, args.count)
        else:
            # Linux / macOS
            with open(src, 'rb') as f:
                f.seek(args.start_sector * 512)
                raw = f.read(args.count * 512)
        print(f"  Read {len(raw):,} bytes from device.")

    else:
        sys.exit(f"ERROR: '{src}' is not a file or a recognised device path.")

    if len(raw) < 512:
        sys.exit("ERROR: Less than one sector of data available.")

    # ── Extract and clean ─────────────────────────────────────────────────────
    print("  Extracting text from sectors…")
    text = extract_text_from_sectors(raw)

    print(f"  Cleaning and validating CSV rows…")
    n = clean_csv(text, args.output)

    print(f"\n  ✅  Done — {n} data rows written to '{args.output}'.\n")


if __name__ == '__main__':
    main()
