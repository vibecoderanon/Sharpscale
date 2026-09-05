#ifndef SHARPSCALE_3DS_H
#define SHARPSCALE_3DS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHARPSCALE_3DS_VERSION_MAJOR 1
#define SHARPSCALE_3DS_VERSION_MINOR 0
#define SHARPSCALE_3DS_VERSION_PATCH 0
#define SHARPSCALE_3DS_VERSION_STRING "1.0.0"

#define SHARPSCALE_3DS_CONFIG_DIR "/luma/sharpscale"
#define SHARPSCALE_3DS_CONFIG_PATH "/luma/sharpscale/config.ini"

/**
 * 3DS Display Modes
 */
typedef enum {
    MODE_3DS_STANDARD_400PX = 0, /**< Standard 400x240 Top Screen (with 3D parallax) */
    MODE_3DS_HIRES_800PX    = 1, /**< 800x240 High-Density 2D Top Screen */
    MODE_3DS_PIXEL_PERFECT  = 2, /**< 1:1 Pixel Mapping for Retro Game Injects / Emulators */
    MODE_3DS_INTEGER_SCALE  = 3  /**< Max Integer Scale Fit with Custom Borders */
} Sharpscale3DSMode;

/**
 * Supported Retro Native Formats for 1:1 Pixel Perfect & Integer scaling on 3DS
 */
typedef enum {
    RETRO_FORMAT_AUTO     = 0,
    RETRO_FORMAT_GBA      = 1, /**< 240x160 (3:2) -> 1x (240x160) or Sharp Fit */
    RETRO_FORMAT_NDS      = 2, /**< 256x192 (4:3) -> 1x (256x192) */
    RETRO_FORMAT_GB_GBC   = 3, /**< 160x144 (10:9) -> 1x or 2x in 800px mode */
    RETRO_FORMAT_GAMEGEAR = 4, /**< 160x144 (10:9) */
    RETRO_FORMAT_NES      = 5, /**< 256x224 (8:7 / 4:3) */
    RETRO_FORMAT_SNES     = 6  /**< 256x224 (8:7 / 4:3) */
} SharpscaleRetroFormat;

/**
 * 3DS Hardware Screen identifiers
 */
typedef enum {
    SCREEN_TOP    = 0,
    SCREEN_BOTTOM = 1
} SharpscaleScreenId;

/**
 * Scaling Filter Profiles
 */
typedef enum {
    FILTER_3DS_POINT          = 0, /**< Nearest Neighbor */
    FILTER_3DS_CRISP_BILINEAR = 1, /**< Sharp-Bilinear (Crisp) */
    FILTER_3DS_SMOOTH         = 2  /**< Stock Bilinear */
} Sharpscale3DSFilter;

typedef struct {
    Sharpscale3DSMode top_screen_mode;
    Sharpscale3DSFilter filter_mode;
    SharpscaleRetroFormat retro_format;
    bool enable_custom_bezel;
    bool wide_3d_mode;
    uint32_t bg_color_rgb888;
} Sharpscale3DSConfig;

void sharpscale_3ds_init(void);
void sharpscale_3ds_exit(void);
Sharpscale3DSConfig* sharpscale_3ds_get_config(void);
void sharpscale_3ds_apply_config(void);

#ifdef __cplusplus
}
#endif

#endif /* SHARPSCALE_3DS_H */
