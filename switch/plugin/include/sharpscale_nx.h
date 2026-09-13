#ifndef SHARPSCALE_NX_H
#define SHARPSCALE_NX_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHARPSCALE_NX_VERSION_MAJOR 1
#define SHARPSCALE_NX_VERSION_MINOR 0
#define SHARPSCALE_NX_VERSION_PATCH 0
#define SHARPSCALE_NX_VERSION_STRING "1.0.0"

#define SHARPSCALE_CONFIG_DIR "/switch/sharpscale"
#define SHARPSCALE_GLOBAL_CONFIG "/switch/sharpscale/config.ini"
#define SHARPSCALE_TITLE_CONFIG_DIR "/switch/sharpscale/titles"

/**
 * Scaling modes supported by Sharpscale-NX
 */
typedef enum {
    SCALING_MODE_ORIGINAL = 0,  /**< Default system scaling (bilinear stretch to display) */
    SCALING_MODE_INTEGER  = 1,  /**< Maximum integer scale (1x, 2x, 3x...) centered with letterboxing */
    SCALING_MODE_REAL     = 2,  /**< 1:1 pixel mapping (unscaled centered framebuffer) */
    SCALING_MODE_FIT      = 3,  /**< Aspect-ratio preserved fit with custom filter */
    SCALING_MODE_CUSTOM   = 4   /**< User-defined viewport width/height and offsets */
} SharpscaleScalingMode;

/**
 * Filtering methods applied during display blit / scanout
 */
typedef enum {
    FILTER_TYPE_POINT           = 0, /**< Nearest Neighbor (sharp crisp pixels, zero blur) */
    FILTER_TYPE_BILINEAR        = 1, /**< Standard hardware bilinear interpolation */
    FILTER_TYPE_SHARP_BILINEAR  = 2, /**< Integer pre-scale + smooth subpixel interpolation */
    FILTER_TYPE_CAS             = 3, /**< AMD Contrast Adaptive Sharpening pass */
    FILTER_TYPE_BICUBIC         = 4  /**< Catmull-Rom bicubic spline filtering */
} SharpscaleFilterType;

/**
 * Aspect ratio overrides
 */
typedef enum {
    ASPECT_RATIO_AUTO    = 0, /**< Use game/source framebuffer aspect ratio */
    ASPECT_RATIO_16_9    = 1, /**< Force 16:9 widescreen */
    ASPECT_RATIO_4_3     = 2, /**< Force 4:3 standard */
    ASPECT_RATIO_3_2     = 3, /**< Force 3:2 (GBA native ratio) */
    ASPECT_RATIO_1_1     = 4, /**< Force 1:1 square pixels */
    ASPECT_RATIO_10_9    = 5  /**< Force 10:9 (Game Boy native ratio) */
} SharpscaleAspectRatio;

/**
 * Viewport rectangle structure
 */
typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} SharpscaleRect;

/**
 * Runtime configuration state
 */
typedef struct {
    uint64_t title_id;
    SharpscaleScalingMode scaling_mode;
    SharpscaleFilterType filter_type;
    SharpscaleAspectRatio aspect_ratio;
    uint8_t sharpness_strength;       /**< 0 to 100% for CAS/sharp filters */
    bool force_1080p_capture;         /**< Unlocks 1080p lossless capture buffer */
    bool show_osd_notification;       /**< Show mode change banner on screen */
    uint32_t border_color_rgba;       /**< RGBA8888 color for letterboxing borders */
    
    /* Source framebuffer metrics */
    uint32_t src_width;
    uint32_t src_height;
    
    /* Target display metrics */
    uint32_t dst_width;
    uint32_t dst_height;
    
    /* Computed output viewport */
    SharpscaleRect calculated_viewport;
    float scale_factor_x;
    float scale_factor_y;
    bool is_docked;
} SharpscaleConfig;

#define SHARPSCALE_SHMEM_MAGIC 0x53485250 // "SHRP"
#define SHARPSCALE_SHMEM_VERSION 1

/**
 * Shared memory layout between SaltyNX plugin (game process)
 * and Tesla Overlay (ovlmenu process).
 */
typedef struct {
    uint32_t magic;           /**< "SHRP" = 0x53485250 */
    uint32_t version;         /**< 1 */
    uint32_t sequence_id;     /**< Increments whenever overlay changes config */
    uint8_t scaling_mode;     /**< SharpscaleScalingMode */
    uint8_t filter_type;      /**< SharpscaleFilterType */
    uint8_t aspect_ratio;     /**< SharpscaleAspectRatio */
    uint8_t sharpness;        /**< 0..100 */
    uint8_t force_1080p;      /**< 0 or 1 */
    uint8_t show_osd;         /**< 0 or 1 */
    uint8_t is_plugin_alive;  /**< 1 when plugin is active in game */
    uint8_t is_docked;        /**< 1 when Switch is docked */
    uint32_t src_width;       /**< Detected game framebuffer width */
    uint32_t src_height;      /**< Detected game framebuffer height */
    uint32_t dst_width;       /**< Output display width */
    uint32_t dst_height;      /**< Output display height */
    uint32_t vp_x;            /**< Calculated viewport X */
    uint32_t vp_y;            /**< Calculated viewport Y */
    uint32_t vp_w;            /**< Calculated viewport W */
    uint32_t vp_h;            /**< Calculated viewport H */
} __attribute__((packed)) SharpscaleSharedMemory;

/**
 * Global API functions
 */
void sharpscale_init(void);
void sharpscale_exit(void);
SharpscaleConfig* sharpscale_get_config(void);
void sharpscale_update_viewport(uint32_t src_w, uint32_t src_h, uint32_t dst_w, uint32_t dst_h);
void sharpscale_apply_settings(void);
void sharpscale_check_live_updates(void);

#ifdef __cplusplus
}
#endif

#endif /* SHARPSCALE_NX_H */
