#!/usr/bin/env python3
"""
Sharpscale-NX: Tesla Overlay Standalone NRO Preview Builder
Fuses LovePotion with the Tesla Menu overlay test wrapper into a bootable .nro.
"""
import os
import shutil
import zipfile

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
# Locate lovepotion.nro from workspace
WORKSPACE_ROOT = os.path.abspath(os.path.join(BASE_DIR, "..", "..", ".."))
LOVEPOTION_SRC = os.path.join(WORKSPACE_ROOT, "deltarune-save-editor", "lovepotion-extracted", "lovepotion.nro")

if not os.path.exists(LOVEPOTION_SRC):
    # Fallback to kickass homebrew
    LOVEPOTION_SRC = os.path.join(WORKSPACE_ROOT, "kickass homebrew", "lovepotion-extracted", "lovepotion.nro")

love_archive = os.path.join(BASE_DIR, "game.love")
output_nro = os.path.join(BASE_DIR, "sharpscale-overlay-preview.nro")

print(f"[*] Packaging main.lua into {love_archive}...")
with zipfile.ZipFile(love_archive, "w", zipfile.ZIP_DEFLATED) as zf:
    zf.write(os.path.join(BASE_DIR, "main.lua"), "main.lua")

print(f"[*] Fusing {LOVEPOTION_SRC} with {love_archive} -> {output_nro}...")
with open(LOVEPOTION_SRC, "rb") as f_base, open(love_archive, "rb") as f_love, open(output_nro, "wb") as f_out:
    f_out.write(f_base.read())
    f_out.write(f_love.read())

print(f"[+] Successfully generated: {output_nro} ({os.path.getsize(output_nro):,} bytes)")
