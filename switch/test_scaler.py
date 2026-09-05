#!/usr/bin/env python3
"""
Test and validation script for Sharpscale-NX scaling algorithms.
Validates integer scale factors, viewport centering, and letterboxing math.
"""

import math

def test_integer_scaling():
    cases = [
        # (src_w, src_h, dst_w, dst_h, expected_scale, expected_w, expected_h)
        (1280, 720, 1920, 1080, 1, 1280, 720),       # 720p on 1080p -> 1x integer scale (centered)
        (960, 540, 1920, 1080, 2, 1920, 1080),       # 540p on 1080p -> 2x integer scale (exact fit!)
        (640, 360, 1280, 720, 2, 1280, 720),         # 360p on 720p  -> 2x integer scale (exact fit!)
        (480, 272, 1280, 720, 2, 960, 544),          # PSP on 720p   -> 2x integer scale (centered)
        (240, 160, 1280, 720, 4, 960, 640),          # GBA on 720p   -> 4x integer scale (centered)
        (256, 224, 1920, 1080, 4, 1024, 896),        # SNES on 1080p -> 4x integer scale (centered)
    ]

    print("[*] Validating Sharpscale-NX Integer Scaler Engine...")
    for src_w, src_h, dst_w, dst_h, exp_scale, exp_w, exp_h in cases:
        scale_x = dst_w // src_w
        scale_y = dst_h // src_h
        scale = min(scale_x, scale_y)
        vp_w = src_w * scale
        vp_h = src_h * scale
        offset_x = (dst_w - vp_w) // 2
        offset_y = (dst_h - vp_h) // 2

        assert scale == exp_scale, f"Scale mismatch for {src_w}x{src_h}: got {scale}, expected {exp_scale}"
        assert vp_w == exp_w, f"Width mismatch: got {vp_w}, expected {exp_w}"
        assert vp_h == exp_h, f"Height mismatch: got {vp_h}, expected {exp_h}"
        assert offset_x >= 0 and offset_y >= 0, "Viewport offset negative!"
        print(f" [+] {src_w}x{src_h} -> {dst_w}x{dst_h}: {scale}x ({vp_w}x{vp_h}) centered at ({offset_x}, {offset_y})")

    print("[+] All scaling tests PASSED successfully!")

if __name__ == "__main__":
    test_integer_scaling()
