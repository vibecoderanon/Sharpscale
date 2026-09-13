# Sharpscale-NX User & Installation Guide

**Sharpscale-NX** provides clean, crisp, and integer-scaled video output for Nintendo Switch games, retro titles, and emulators.

---

## Features
- **Integer Scaling**: Automatically detects source render resolution (e.g. 720p, 540p, 480p, 360p) and applies maximum integer upscale to 1080p (Docked) or 720p (Handheld) with letterboxing.
- **Real (1:1 Native Centering)**: Centers the native game framebuffer with 1:1 pixel mapping.
- **Filtering Options**:
  - **Point (Nearest Neighbor)**: Raw crisp pixel art.
  - **Sharp Bilinear**: Pixel-perfect look with subpixel anti-shimmer.
  - **AMD CAS (Contrast Adaptive Sharpening)**: GPU post-processing pass that sharpens textures and edges without ringing.
  - **Bicubic Spline**: High-fidelity smooth scaling.
- **Aspect Ratio Override**: Force 16:9, 4:3, 3:2 (GBA), 10:9 (GB/GBC), or 1:1 square pixels.
- **Lossless 1080p Stream Capture**: Unlocks full-resolution framebuffers for `SysDVR` / USB streaming.
- **Tesla Overlay**: Real-time configuration menu accessible anywhere in-game.

---

## Requirements & Prerequisites

> [!IMPORTANT]
> **SaltyNX and Tesla Menu must be installed first.** Sharpscale-NX does not bundle the SaltyNX background daemon or the Tesla loader. You must have both installed before copying Sharpscale-NX files.

- Nintendo Switch running **[Atmosphère Custom Firmware](https://github.com/Atmosphere-NX/Atmosphere)** (1.5.0 or newer).
- **[SaltyNX](https://github.com/masagrator/SaltyNX)** (Required for title-level NVN / VI GPU interception; installed to `atmosphere/contents/0000000000534C54/`).
- **Tesla Menu Environment** (for the on-screen configuration overlay):
  - **[nx-ovlloader](https://github.com/WerWolv/nx-ovlloader)** (Sysmodule overlay host).
  - **[Tesla-Menu / ovlmenu](https://github.com/WerWolv/Tesla-Menu)** (Overlay menu front-end).

---

## Installation

### 1. SaltyNX Plugin Installation
1. Copy `sharpscale.elf` to `sdmc:/SaltySD/plugins/sharpscale.elf`.
2. Ensure SaltyNX is enabled in `sdmc:/SaltySD/flags/`.

### 2. Tesla Overlay Installation
1. Copy `ovl-sharpscale.ovl` to `sdmc:/switch/.overlays/ovl-sharpscale.ovl`.
2. Open the Tesla Menu at any time using the button combo:
   $$\text{L} + \text{D-Pad Down} + \text{R3 (Right Stick Click)}$$
3. Select **Sharpscale-NX** to configure settings in real time.

---

## Configuration Files

Configuration is stored in INI format under `/switch/sharpscale/`:
- Global settings: `sdmc:/switch/sharpscale/config.ini`
- Per-title overrides: `sdmc:/switch/sharpscale/titles/<TitleID>.ini`

### Sample `config.ini`:
```ini
# Sharpscale-NX Configuration
scaling_mode = integer          # Options: original, integer, real, fit, custom
filter_type = cas               # Options: point, bilinear, sharp_bilinear, cas, bicubic
aspect_ratio = auto             # Options: auto, 16:9, 4:3, 3:2, 1:1, 10:9
sharpness = 80                  # 0 to 100%
force_1080p_capture = 1         # 1 = Enabled, 0 = Disabled
show_osd = 1                    # 1 = Show mode banner on launch
```

---

## Building from Source

```bash
# Set up devkitPro environment
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=$DEVKITPRO/devkitARM
export DEVKITA64=$DEVKITPRO/devkitA64

# Build plugin and overlay
cd switch
make
```
