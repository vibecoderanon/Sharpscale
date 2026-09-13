# Sharpscale-3DS User & Installation Guide

**Sharpscale-3DS** removes the blurry scaling filters from the Nintendo 3DS, enabling crisp 1:1 pixel rendering, 800px high-density progressive mode, and patched hardware scaling matrices for Nintendo DS (`TWL_FIRM`) and Game Boy Advance (`AGB_FIRM`) modes.

---

## Features
- **800px High-Density 2D Mode**: Unlocks the full $800 \times 240$ physical subpixel resolution of the top screen in 2D mode, doubling horizontal clarity for retro emulators and text rendering.
- **Hardware Scaling Matrix Patcher**: Replaces the default blurry 5-tap polyphase filter tables in `TWL_FIRM` (DS) and `AGB_FIRM` (GBA) with:
  - **Point (Nearest Neighbor)**: Raw pixel grid.
  - **Sharp Bilinear (Crisp)**: Smooth subpixel interpolation without overall blur.
  - **Catmull-Rom Bicubic**: Smooth high-fidelity scaling.
- **1:1 Pixel-Perfect Framing & Bezels**: Centered viewport frames with custom border graphics for:
  - Game Boy Advance ($240 \times 160$)
  - Nintendo DS ($256 \times 192$)
  - Game Boy / Game Boy Color ($160 \times 144$)
  - Sega Game Gear ($160 \times 144$)
  - NES / SNES ($256 \times 224$)
- **Luma3DS 3GX Plugin**: Real-time mode switching and HUD control.
- **Configurator Homebrew App (`.3dsx` / `.cia`)**: Easy on-device GUI for toggling options and auto-generating patched FIRM binaries.

---

## Requirements & Prerequisites
- Nintendo 3DS / New 3DS / 2DS running **[Luma3DS](https://github.com/LumaTeam/Luma3DS)** custom firmware (v10.0 or newer).
- Luma3DS **"Enable loading external FIRMs and modules"** option enabled in Luma boot config (`Hold SELECT on boot`).
- Luma3DS **"Enable plugin loader"** option enabled for the 3GX plugin (in Rosalina menu: `L + D-Pad Down + Select`).

---

## Installation

### 1. In-Game 3GX Plugin
1. Copy `sharpscale.3gx` to `sdmc:/luma/plugins/default.3gx` (or to `sdmc:/luma/plugins/<TitleID>/sharpscale.3gx` for specific titles).
2. Open the Luma Rosalina menu (`L + D-Pad Down + Select`) and ensure **Plugin Loader** is enabled.

### 2. TWL (DS) & AGB (GBA) Scaling Matrix Patching
1. Run `Sharpscale-3DS.3dsx` from the Homebrew Launcher (or install `Sharpscale-3DS.cia`).
2. Select **"Generate Patched TWL_FIRM"** or **"Generate Patched AGB_FIRM"**.
3. Choose your desired filter profile (**Point** or **Sharp Bilinear**).
4. The tool will automatically place the patched FIRMs into `sdmc:/luma/sysmodules/twl.firm` and `sdmc:/luma/sysmodules/agb.firm`.
5. Reboot and enjoy razor-sharp DS and GBA playback!

---

## CLI FIRM Patcher (PC / Mac / Linux)

You can also patch FIRM dumps on your computer using the standalone CLI:

```bash
# Patch TWL (DS) FIRM with Sharp Bilinear filter:
./firm_patcher twl sharp twl.firm sdmc/luma/sysmodules/twl.firm

# Patch AGB (GBA) FIRM with Point (Nearest Neighbor) filter:
./firm_patcher agb point agb.firm sdmc/luma/sysmodules/agb.firm
```

---

## Building from Source

```bash
# Set up devkitPro environment
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=$DEVKITPRO/devkitARM

# Build all 3DS targets (3GX plugin, FIRM patcher, and Configurator app)
cd 3ds
make
```
