"""
Sharpscale Multi-Console Suite: Standalone Release Packager
Packages genuine compiled production console binaries into the consolidated SD card release archive:
- Sharpscale-NX SaltyNX Plugin (sharpscale.elf -> sdmc:/SaltySD/plugins/)
- Sharpscale-NX Tesla Overlay (ovl-sharpscale.ovl -> sdmc:/switch/.overlays/)
- Sharpscale-3DS Luma3DS In-Game Plugin (sharpscale.3gx -> sdmc:/luma/plugins/)
- Sharpscale-3DS Configurator Homebrew (Sharpscale-3DS.3dsx -> sdmc:/3ds/)
- Sharpscale FIRM Matrix Patcher (sharpscale_firm_patcher.py -> sdmc:/3ds/)
- Consolidated SD Card Distribution Zip (Sharpscale-MultiConsole-Suite.zip)
"""

import os
import shutil
import zipfile

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
RELEASE_DIR = os.path.join(BASE_DIR, "release")
DIST_DIR = os.path.join(BASE_DIR, "dist")

def package_suite():
    print("[*] Packaging Sharpscale Multi-Console Suite...")

    # SD Card directory layout
    switch_overlay_dir = os.path.join(RELEASE_DIR, "switch", ".overlays")
    saltysd_plugin_dir = os.path.join(RELEASE_DIR, "SaltySD", "plugins")
    ctr_app_dir = os.path.join(RELEASE_DIR, "3ds")
    luma_plugin_dir = os.path.join(RELEASE_DIR, "luma", "plugins")

    for d in [switch_overlay_dir, saltysd_plugin_dir, ctr_app_dir, luma_plugin_dir]:
        os.makedirs(d, exist_ok=True)

    # Artifact sources (check dist, compiled source trees, or pre-existing release binaries)
    artifact_mappings = [
        # (Candidate search paths, Destination path)
        (
            [
                os.path.join(DIST_DIR, "switch", "ovl-sharpscale.ovl"),
                os.path.join(DIST_DIR, "switch", "overlay", "ovl-sharpscale.ovl"),
                os.path.join(BASE_DIR, "switch", "overlay", "ovl-sharpscale.ovl"),
                os.path.join(switch_overlay_dir, "ovl-sharpscale.ovl"),
            ],
            os.path.join(switch_overlay_dir, "ovl-sharpscale.ovl")
        ),
        (
            [
                os.path.join(DIST_DIR, "switch", "sharpscale.elf"),
                os.path.join(DIST_DIR, "switch", "plugin", "sharpscale.elf"),
                os.path.join(BASE_DIR, "switch", "plugin", "sharpscale.elf"),
                os.path.join(saltysd_plugin_dir, "sharpscale.elf"),
            ],
            os.path.join(saltysd_plugin_dir, "sharpscale.elf")
        ),
        (
            [
                os.path.join(DIST_DIR, "3ds", "Sharpscale-3DS.3dsx"),
                os.path.join(DIST_DIR, "3ds", "config_app", "Sharpscale-3DS.3dsx"),
                os.path.join(BASE_DIR, "3ds", "config_app", "Sharpscale-3DS.3dsx"),
                os.path.join(ctr_app_dir, "Sharpscale-3DS.3dsx"),
            ],
            os.path.join(ctr_app_dir, "Sharpscale-3DS.3dsx")
        ),
        (
            [
                os.path.join(DIST_DIR, "3ds", "sharpscale.3gx"),
                os.path.join(DIST_DIR, "3ds", "plugin_3gx", "sharpscale.3gx"),
                os.path.join(BASE_DIR, "3ds", "plugin_3gx", "sharpscale.3gx"),
                os.path.join(luma_plugin_dir, "sharpscale.3gx"),
            ],
            os.path.join(luma_plugin_dir, "sharpscale.3gx")
        ),
        (
            [
                os.path.join(BASE_DIR, "3ds", "firm_patcher", "sharpscale_firm_patcher.py"),
            ],
            os.path.join(ctr_app_dir, "sharpscale_firm_patcher.py")
        ),
    ]

    for candidates, dest in artifact_mappings:
        src = next((p for p in candidates if os.path.exists(p)), None)
        if src and src != dest:
            shutil.copy2(src, dest)
            print(f" [+] Bundled: {os.path.basename(dest)} ({os.path.getsize(dest):,} bytes)")
        elif os.path.exists(dest):
            print(f" [+] Verified existing: {os.path.basename(dest)} ({os.path.getsize(dest):,} bytes)")
        else:
            print(f" [!] Warning: Target {os.path.basename(dest)} not found in build outputs.")

    # Generate clean all-in-one zip archive
    zip_path = os.path.join(RELEASE_DIR, "Sharpscale-MultiConsole-Suite.zip")
    if os.path.exists(zip_path):
        os.remove(zip_path)

    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
        for root, _, files in os.walk(RELEASE_DIR):
            for file in files:
                if file.endswith(".zip"):
                    continue
                full_p = os.path.join(root, file)
                rel_p = os.path.relpath(full_p, RELEASE_DIR)
                zf.write(full_p, rel_p)
                print(f"  -> Added to SD archive: {rel_p}")

    print(f"\n[+] Successfully packaged SD release bundle: {zip_path} ({os.path.getsize(zip_path):,} bytes)")

if __name__ == "__main__":
    package_suite()
