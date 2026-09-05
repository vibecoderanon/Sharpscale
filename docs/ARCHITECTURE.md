# Sharpscale Architecture & Display Pipeline Deep-Dive

This document details the low-level graphics and display architectures of the **PlayStation Vita**, **Nintendo Switch**, and **Nintendo 3DS**, explaining why default scaling algorithms degrade image quality and how **Sharpscale** resolves these limitations across each platform.

---

## 1. PlayStation Vita & PlayStation TV (Reference Implementation)

### Hardware Architecture
- **Handheld PS Vita**: 5-inch OLED/LCD with native $960 \times 544$ resolution ($16:9$).
- **PlayStation TV (VTE-1000)**: Outputs digital video via HDMI at $1280 \times 720$ ($720\text{p}$) or $1920 \times 1080$ ($1080\text{i}$).
- **Display Controller**: Hardware 2D Video Scaler inside the Sony/Toshiba SoC.

### The Scaling Problem on Vita / PSTV
1. **PSTV 720p Output**: The PSTV takes the native $960 \times 544$ buffer and upscales it by $\approx 1.333\times$ to $1280 \times 720$. Sony's firmware uses a fixed 2D bilinear hardware filter, causing noticeable softness and edge blurring on HDTVs.
2. **PSTV 1080i Output**: Multiplying $544 \times 2 = 1088$ lines. Since $1088 > 1080$, Sony downsampled or bilinearly resampled the buffer instead of offering an integer crop.
3. **PSP / PS1 Emulation (Adrenaline)**: PSP games render at $480 \times 272$. On PSTV, this suffered double bilinear filtering: first $272 \to 544$, then $544 \to 720\text{p}/1080\text{i}$.

### How Vita Sharpscale Solves It
- Hooks `SceDisplay` and low-level display registers via **taiHEN** kernel mode.
- Replaces bilinear filter registers with **Point (Nearest Neighbor)** filtering.
- Implements **Integer Scaling** (cropping 8 lines in 1080i to achieve a razor-sharp $2\times$ scale) and **Real (1:1 Native Centering)** with black borders.
- Unlocks the display driver to accept arbitrary custom framebuffers ($1280 \times 720$, $1920 \times 1080$) for USB streaming (`vita-udcd-uvc`).

---

## 2. Nintendo Switch (`Sharpscale-NX`)

```
+-----------------------------------------------------------------------------------------+
|                                NINTENDO SWITCH DISPLAY PIPELINE                         |
+=========================================================================================+
| [Game Engine / Emulator]                                                                |
|       | (Renders at e.g. 540p, 720p, 900p, or 1080p)                                   |
|       v                                                                                 |
| [NVN / Vulkan / EGL Graphics API] <--- [Sharpscale-NX SaltyNX Hook (Swapchain/CAS)]    |
|       |                                                                                 |
|       v                                                                                 |
| [VI Service (nvnflinger / compositor)] <--- [Sharpscale-NX Layer Cropping & Centering]  |
|       |                                                                                 |
|       v                                                                                 |
| [Tegra X1 DC (Display Controller) / VIC]                                                |
|       |                                                                                 |
|       +---> Handheld LCD/OLED: 1280x720 (Point / Integer / CAS Sharpened)               |
|       +---> Docked HDMI Output: 1920x1080 (Integer 2x / Real 1:1 / CAS Sharpened)       |
+-----------------------------------------------------------------------------------------+
```

### Hardware Architecture
- **Handheld Display**: $1280 \times 720$ ($720\text{p}$) 60Hz 16:9.
- **Docked Output**: $1920 \times 1080$ ($1080\text{p}$) / $1280 \times 720$ HDMI.
- **SoC**: Nvidia Tegra X1 (T210 / T210B01) with Maxwell GM20B GPU and Tegra Display Controller (DC).

### The Scaling Problem on Switch
1. **Dynamic Resolution Scaling (DRS) & Sub-Native Handheld Games**: Many 3D titles render at sub-720p resolutions (e.g. $540\text{p}$, $480\text{p}$, or $360\text{p}$) and apply simple bilinear stretches, resulting in visual blur.
2. **Docked 720p Titles on 1080p Displays**: Non-1080p games are scaled by the Tegra Display Controller or VIC polyphase filter.
3. **Retro Emulation & Classic Games**: Nintendo Switch Online (NSO) emulators (NES, SNES, GBA, N64, Genesis) apply bilinear filtering by default without allowing global integer scaling overrides or custom pixel-art shaders.

### How Sharpscale-NX Solves It
- **SaltyNX / NVN Swapchain Interception**: Intercepts `nvnQueuePresentTexture`, `nvnWindowSetCrop`, and `viSetLayerScalingMode`.
- **Integer Scaling Mode**: Calculates exact maximum integer multiplier ($1\times, 2\times, 3\times$) for the game's source resolution and centers the output on the display.
- **Real Mode (1:1 Pixel Mapping)**: Centers the native frame without resampling.
- **AMD Contrast Adaptive Sharpening (CAS)**: Executes a high-performance compute/fragment sharpening pass to restore edge clarity without ringing artifacts.
- **Tesla Overlay (`ovl-sharpscale`)**: Allows real-time toggling of scaling modes, aspect ratio overrides ($16:9, 4:3, 3:2, 10:9, 1:1$), and filter settings.

---

## 3. Nintendo 3DS (`Sharpscale-3DS`)

```
+-----------------------------------------------------------------------------------------+
|                                 NINTENDO 3DS DISPLAY PIPELINE                           |
+=========================================================================================+
| [CTR 3DS Mode]                 | [TWL DS Mode (TWL_FIRM)]  | [AGB GBA Mode (AGB_FIRM)]  |
| 400x240 (3D) or 800x240 (2D)   | Native: 256x192           | Native: 240x160            |
|       |                        |       |                   |       |                    |
|       v                        |       v                   |       v                    |
| [GSPGPU / LCD Controller]      | [Polyphase 5-Tap Scaler]  | [Polyphase 5-Tap Scaler]   |
|   |                            |   |                       |   |                        |
|   +-> 800px Mode Driver        |   +-> Patched Matrix      |   +-> Patched Matrix       |
|       (Dual-Column 2D Scanout) |       (Point / Sharp Bilin|       (Point / Sharp Bilin|
|                                |        Bicubic Spline)    |        1:1 Bezel Centered) |
+--------------------------------+---------------------------+----------------------------+
```

### Hardware Architecture
- **Top Display**: $800 \times 240$ physical subpixel columns ($400 \times 240$ per eye with parallax barrier active, or $800 \times 240$ progressive 2D in high-density mode).
- **Bottom Display**: $320 \times 240$ resistive touchscreen.
- **SoC**: ARM11 MPCore + DMP PICA200 GPU + ARM9 security processor + hardware polyphase 2D scaler.

### The Scaling Problem on 3DS
1. **DS Games in TWL_FIRM ($256 \times 192$)**:
   - $400 / 256 = 1.5625\times$ horizontal stretch.
   - $240 / 192 = 1.25\times$ vertical stretch.
   - Stock firmware applies a blurry 5-tap bilinear matrix that obscures fine pixel art.
2. **GBA Games in AGB_FIRM ($240 \times 160$)**:
   - $400 / 240 = 1.666\times$ horizontal stretch.
   - $240 / 160 = 1.5\times$ vertical stretch.
   - Stock firmware applies an uneven hardware stretch that produces shimmer during horizontal scrolling.
3. **Top Screen Resolution Underutilization**: The top screen contains 800 horizontal subpixels, but native 2D mode only addresses 400.

### How Sharpscale-3DS Solves It
- **Hardware Matrix Patcher**: Replaces the 80-byte 5-tap polyphase filter tables in `TWL_FIRM` and `AGB_FIRM` with **Point** (nearest neighbor) or **Sharp-Bilinear** matrices.
- **800px High-Density 2D Mode**: Activates progressive 800-column scanout by interleaving even/odd columns across the dual framebuffer pipes, doubling horizontal resolution.
- **Pixel-Perfect Retro Framing & Bezels**: Centers retro formats ($240 \times 160, 256 \times 192, 160 \times 144$) with styled letterboxing and border art.
- **Luma3DS 3GX Plugin & Configurator App**: Real-time mode switching and automated FIRM generation.
