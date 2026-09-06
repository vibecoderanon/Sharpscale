# Sharpscale Multi-Console Suite

[![Platform: PS Vita / PSTV](https://img.shields.io/badge/Platform-PS_Vita_%2F_PSTV-blue.svg)](https://github.com/Electry/Sharpscale)
[![Platform: Nintendo Switch](https://img.shields.io/badge/Platform-Nintendo_Switch-red.svg)](docs/SWITCH_GUIDE.md)
[![Platform: Nintendo 3DS](https://img.shields.io/badge/Platform-Nintendo_3DS-orange.svg)](docs/3DS_GUIDE.md)
[![License: MIT / GPLv3](https://img.shields.io/badge/License-GPLv3-green.svg)](LICENSE)
[![Web Portal](https://img.shields.io/badge/Web_Portal-View_Suite-3b82f6?style=for-the-badge&logo=googlechrome&logoColor=white)](https://vibecoderanon.github.io/#sharpscale)

**Sharpscale** is a suite of homebrew display plugins and utilities designed to eliminate blurry hardware scaling filters and deliver crisp, clean, and pixel-perfect video output across game consoles.

Originally created for the PlayStation Vita / PlayStation TV by [Electry](https://github.com/Electry/Sharpscale) and [cuevavirus](https://github.com/cuevavirus/sharpscale), this repository provides full-featured implementations and architectural adaptations for:
1. **Nintendo Switch (`Sharpscale-NX`)**
2. **Nintendo 3DS (`Sharpscale-3DS`)**

---

## Quick Navigation

- [Online Web Portal](#online-web-portal)
- [Architecture & Display Subsystems Deep-Dive](docs/ARCHITECTURE.md)
- [Nintendo Switch Guide & Installation](docs/SWITCH_GUIDE.md)
- [Nintendo 3DS Guide & Installation](docs/3DS_GUIDE.md)

---

## Online Web Portal

Access the web portal documentation, console capture showcase, and multi-console release packages directly at:
**[https://vibecoderanon.github.io/#sharpscale](https://vibecoderanon.github.io/#sharpscale)**

---

## Feature Comparison Matrix

| Feature | PS Vita / PSTV (Original) | Nintendo Switch (`Sharpscale-NX`) | Nintendo 3DS (`Sharpscale-3DS`) |
| :--- | :--- | :--- | :--- |
| **Integer Scaling** | Yes ($2\times$ in 1080i with 8px crop) | Yes (Auto-calculated max fit for 1080p/720p) | Yes (Centered integer scale with custom bezels) |
| **Real Mode (1:1 Native Centering)** | Yes ($960\times544$ centered in 720p) | Yes ($720\text{p}/540\text{p}/480\text{p}/360\text{p}$ 1:1 mapping) | Yes ($240\times160$ GBA, $256\times192$ DS 1:1) |
| **Point (Nearest Neighbor) Filtering** | Yes (Hardware register override) | Yes (Swapchain texture sampler override) | Yes (5-tap polyphase matrix replacement) |
| **Sharp Bilinear / Subpixel Crisp** | Yes (Software/Hardware filter) | Yes (Integrated shader pass) | Yes (Polyphase matrix coefficients) |
| **Advanced Post-Processing** | Bilinear / Point | AMD Contrast Adaptive Sharpening (CAS), Bicubic | Bicubic Spline, Point, Sharp Bilinear |
| **High-Resolution Progressive Mode** | Custom framebuffers ($1080\text{p}$) | Unlocked 1080p lossless capture (`SysDVR`) | $800\times240$ Progressive 2D Mode (Top Screen) |
| **In-Game Overlay / Menu** | Standalone VPK Configurator | Tesla Menu Overlay (`ovl-sharpscale`) | Luma3DS 3GX Plugin + Configurator App |

---

## Repository Structure

```
Sharpscale/
├── README.md
├── docs/
│   ├── ARCHITECTURE.md                  # Comprehensive display pipeline comparison
│   ├── SWITCH_GUIDE.md                  # Switch installation and Tesla guide
│   └── 3DS_GUIDE.md                     # 3DS installation, 800px mode & FIRM patcher
├── switch/
│   ├── Makefile
│   ├── plugin/                          # SaltyNX NVN/VI Hook Plugin (sharpscale.elf)
│   │   ├── include/                     # Plugin headers
│   │   └── src/                         # Plugin implementation (NVN, VI, Scaler, CAS)
│   └── overlay/                         # Tesla Menu Overlay (ovl-sharpscale.ovl)
│       ├── include/
│       └── src/
└── 3ds/
    ├── Makefile
    ├── plugin_3gx/                      # Luma3DS 3GX In-Game Plugin (sharpscale.3gx)
    │   ├── include/
    │   └── src/                         # 800px driver, GSPGPU hooks, Bezel engine
    ├── firm_patcher/                    # TWL/AGB Kernel Polyphase Matrix Patcher
    │   ├── include/
    │   └── src/
    └── config_app/                      # 3DS Homebrew Configurator App (Sharpscale-3DS.3dsx/.cia)
        ├── include/
        └── src/
```

---

## Building All Platforms

### Prerequisites
- [devkitPro](https://devkitpro.org/) with `devkitA64` (Switch) and `devkitARM` (3DS).
- `libnx`, `libtesla`, `libctru`, `3dsxtool`, and `3gxtool`.

### Build Commands

```bash
# Build Nintendo Switch targets (SaltyNX plugin and Tesla overlay)
cd switch
make

# Build Nintendo 3DS targets (3GX plugin, FIRM patcher, and 3DSX app)
cd ../3ds
make
```

---

## Credits & Acknowledgments
- **Electry** & **cuevavirus**: Creators of the original [Sharpscale](https://github.com/Electry/Sharpscale) for PS Vita / PSTV.
- **xerpi**: PS Vita display reverse engineering (`SceDisplay` / `SceLowio`).
- **devkitPro team**: Homebrew toolchains and libraries for Switch (`libnx`) and 3DS (`libctru`).
- **WerWolv**: Tesla menu environment for Nintendo Switch.
- **Luma3DS team**: Custom firmware and 3GX plugin loader for Nintendo 3DS.
