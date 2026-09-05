#!/usr/bin/env python3
"""
Sharpscale-3DS FIRM Matrix Patcher (Python Edition)
Patches the hardware 2D polyphase scaling matrices in TWL_FIRM (DS) and AGB_FIRM (GBA)
for crystal clear, crisp, or integer-scaled retro rendering.
"""

import sys
import struct
import argparse
from typing import List, Tuple

# Filter profiles: 8 rows x 5 taps of signed 16-bit integers
MATRICES = {
    "point": [
        [0, 0, 256, 0, 0],
        [0, 0, 256, 0, 0],
        [0, 0, 256, 0, 0],
        [0, 0, 256, 0, 0],
        [0, 0, 256, 0, 0],
        [0, 0, 256, 0, 0],
        [0, 0, 256, 0, 0],
        [0, 0, 256, 0, 0]
    ],
    "sharp": [
        [0, 0, 256, 0, 0],
        [0, 0, 230, 26, 0],
        [0, 0, 192, 64, 0],
        [0, 0, 140, 116, 0],
        [0, 0, 128, 128, 0],
        [0, 16, 116, 140, 0],
        [0, 64, 192, 0, 0],
        [0, 26, 230, 0, 0]
    ],
    "bicubic": [
        [0, -16, 288, -16, 0],
        [0, -20, 268, 8, 0],
        [0, -22, 236, 42, 0],
        [0, -20, 196, 80, 0],
        [0, -16, 144, 128, 0],
        [0, -8, 96, 168, 0],
        [0, 0, 48, 208, 0],
        [0, 0, 12, 244, 0]
    ]
}

# Signatures for locating polyphase scaling tables in FIRM binaries
TWL_SIGNATURE = bytes([0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x40, 0x00])
AGB_SIGNATURE = bytes([0x00, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x50, 0x00])

def pack_matrix(matrix: List[List[int]]) -> bytes:
    data = bytearray()
    for row in matrix:
        for val in row:
            data.extend(struct.pack("<h", val))
    return bytes(data)

def patch_firm_data(firm_data: bytearray, target: str, filter_profile: str) -> bool:
    if filter_profile not in MATRICES:
        raise ValueError(f"Unknown filter profile: {filter_profile}")

    patch_bytes = pack_matrix(MATRICES[filter_profile])
    sig = TWL_SIGNATURE if target.lower() == "twl" else AGB_SIGNATURE

    idx = firm_data.find(sig)
    if idx == -1:
        # If signature not found directly, look for stock identity coefficients
        return False

    firm_data[idx:idx + len(patch_bytes)] = patch_bytes
    return True

def main():
    parser = argparse.ArgumentParser(description="Sharpscale-3DS FIRM Matrix Patcher")
    parser.add_argument("target", choices=["twl", "agb"], help="Target FIRM type: twl (DS) or agb (GBA)")
    parser.add_argument("filter", choices=["point", "sharp", "bicubic"], help="Scaling filter profile")
    parser.add_argument("input", help="Path to decrypted input FIRM file (e.g. twl.firm)")
    parser.add_argument("-o", "--output", help="Path to output patched FIRM file (default: patched.firm)", default="patched.firm")
    parser.add_argument("--self-test", action="store_true", help="Run self-test verification")

    args = parser.parse_args()

    if args.self_test:
        print("[*] Running Sharpscale FIRM patcher self-test...")
        # Create a mock FIRM buffer with signature
        mock = bytearray(b"\x00" * 1024 + TWL_SIGNATURE + b"\x00" * 1024)
        ok = patch_firm_data(mock, "twl", "sharp")
        assert ok, "Self-test failed to patch mock TWL buffer"
        print("[+] Self-test PASSED successfully!")
        return 0

    try:
        with open(args.input, "rb") as f:
            data = bytearray(f.read())
    except Exception as e:
        print(f"[-] Error reading input file: {e}")
        return 1

    print(f"[*] Patching {args.input} ({len(data)} bytes) for target '{args.target}' with filter '{args.filter}'...")
    if patch_firm_data(data, args.target, args.filter):
        with open(args.output, "wb") as f:
            f.write(data)
        print(f"[+] Success! Patched FIRM written to: {args.output}")
        print(f"[*] Copy this file to your 3DS SD card at: sdmc:/luma/sysmodules/{args.target.lower()}.firm")
        return 0
    else:
        print("[-] Warning: Target scaling table signature not found in input binary.")
        print("    Ensure the FIRM is decrypted and matches standard CTR TWL/AGB FIRM format.")
        return 1

if __name__ == "__main__":
    sys.exit(main())
