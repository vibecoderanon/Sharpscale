#!/usr/bin/env python3
"""
Automated Switch Homebrew Screen Capture Tool
Uses Ryujinx native F8 screenshot trigger to grab a pristine 1:1 lossless framebuffer capture.
"""
import os
import sys
import time
import glob
import shutil
import subprocess
import ctypes
from ctypes import wintypes

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
WORKSPACE_ROOT = os.path.abspath(os.path.join(BASE_DIR, "..", "..", ".."))

RYUJINX_PATH = os.path.expandvars(
    r"%USERPROFILE%\Documents\sw\switch\ryujinx-1.1.1403-win_x64\publish\Ryujinx.exe"
)
TARGET_NRO = os.path.join(BASE_DIR, "sharpscale-overlay-preview.nro")
SCREENSHOTS_DIR = os.path.expandvars(r"%APPDATA%\Ryujinx\screenshots")

OUTPUT_DOCS = os.path.abspath(os.path.join(BASE_DIR, "..", "..", "docs", "sharpscale_overlay_preview.png"))
OUTPUT_PORTAL = os.path.join(WORKSPACE_ROOT, "vibecoderanon.github.io", "apps", "sharpscale", "sharpscale_preview.png")

print(f"[*] Target NRO: {TARGET_NRO}")
print(f"[*] Emulator: {RYUJINX_PATH}")
os.makedirs(SCREENSHOTS_DIR, exist_ok=True)

# List initial screenshots
existing_screenshots = set(glob.glob(os.path.join(SCREENSHOTS_DIR, "*.png")))

# 1. Spawn emulator process
print("[*] Launching Ryujinx with target homebrew...")
proc = subprocess.Popen([RYUJINX_PATH, TARGET_NRO])

# 2. Wait for homebrew engine and window to initialize
wait_time = 12
print(f"[*] Waiting {wait_time} seconds for title screen rendering...")
time.sleep(wait_time)

# 3. Locate emulator window handle (HWND)
user32 = ctypes.windll.user32
found_hwnds = []

def enum_windows_callback(hwnd, extra):
    if user32.IsWindowVisible(hwnd):
        length = user32.GetWindowTextLengthW(hwnd)
        if length > 0:
            buff = ctypes.create_unicode_buffer(length + 1)
            user32.GetWindowTextW(hwnd, buff, length + 1)
            title = buff.value
            if "Ryujinx" in title:
                found_hwnds.append((hwnd, title))
    return True

WNDENUMPROC = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
user32.EnumWindows(WNDENUMPROC(enum_windows_callback), 0)

if found_hwnds:
    target_hwnd, win_title = found_hwnds[0]
    print(f"[*] Targeting HWND {target_hwnd}: '{win_title}'")
    
    user32.ShowWindow(target_hwnd, 9) # SW_RESTORE
    user32.SetForegroundWindow(target_hwnd)
    time.sleep(1)

    VK_F8 = 0x77
    WM_KEYDOWN = 0x0100
    WM_KEYUP = 0x0101
    
    print("[*] Sending F8 screenshot commands to Ryujinx...")
    for i in range(3):
        user32.keybd_event(VK_F8, 0, 0, 0)
        user32.PostMessageW(target_hwnd, WM_KEYDOWN, VK_F8, 0)
        time.sleep(0.15)
        user32.keybd_event(VK_F8, 0, 2, 0)
        user32.PostMessageW(target_hwnd, WM_KEYUP, VK_F8, 0)
        time.sleep(0.8)
    
    # Wait for screenshot to write to disk
    time.sleep(3)
else:
    print("[!] Warning: Ryujinx window not found.")

# 4. Gracefully terminate emulator
print("[*] Terminating emulator process...")
proc.terminate()
try:
    proc.wait(timeout=3)
except subprocess.TimeoutExpired:
    proc.kill()

# 5. Check for new screenshot
current_screenshots = set(glob.glob(os.path.join(SCREENSHOTS_DIR, "*.png")))
new_screenshots = list(current_screenshots - existing_screenshots)

if new_screenshots:
    new_shots_sorted = sorted(new_screenshots, key=os.path.getmtime, reverse=True)
    latest = new_shots_sorted[0]
    print(f"[+] Captured Ryujinx screenshot: {latest} ({os.path.getsize(latest):,} bytes)")

    os.makedirs(os.path.dirname(OUTPUT_DOCS), exist_ok=True)
    shutil.copy2(latest, OUTPUT_DOCS)
    print(f"[+] Saved to docs: {OUTPUT_DOCS}")

    os.makedirs(os.path.dirname(OUTPUT_PORTAL), exist_ok=True)
    shutil.copy2(latest, OUTPUT_PORTAL)
    print(f"[+] Saved to portal: {OUTPUT_PORTAL}")
else:
    print(f"[!] No new screenshot detected in {SCREENSHOTS_DIR}.")
    # Check all files in directory
    all_shots = glob.glob(os.path.join(SCREENSHOTS_DIR, "*.png"))
    print(f"[*] Total files in {SCREENSHOTS_DIR}: {len(all_shots)}")
