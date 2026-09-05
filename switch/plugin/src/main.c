#include "sharpscale_nx.h"
#include "nvn_hook.h"
#include "vi_hook.h"
#include "scaler.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

static SharpscaleConfig g_config;
static bool g_initialized = false;

SharpscaleConfig* sharpscale_get_config(void) {
    return &g_config;
}

void sharpscale_update_viewport(uint32_t src_w, uint32_t src_h, uint32_t dst_w, uint32_t dst_h) {
    g_config.src_width = src_w;
    g_config.src_height = src_h;
    g_config.dst_width = dst_w;
    g_config.dst_height = dst_h;
    g_config.is_docked = vi_hook_is_docked();

    scaler_calculate_viewport(
        src_w,
        src_h,
        dst_w,
        dst_h,
        g_config.scaling_mode,
        g_config.aspect_ratio,
        &g_config.calculated_viewport,
        &g_config.scale_factor_x,
        &g_config.scale_factor_y
    );
}

void sharpscale_apply_settings(void) {
    if (!g_initialized) return;

    /* Re-evaluate display mode */
    g_config.is_docked = vi_hook_is_docked();
    uint32_t dst_w = g_config.is_docked ? 1920 : 1280;
    uint32_t dst_h = g_config.is_docked ? 1080 : 720;

    if (g_config.src_width > 0 && g_config.src_height > 0) {
        sharpscale_update_viewport(g_config.src_width, g_config.src_height, dst_w, dst_h);
    }

    switch (g_config.scaling_mode) {
        case SCALING_MODE_ORIGINAL:
            vi_hook_set_scaling_mode(VI_SCALING_MODE_FIT_TO_LAYER);
            break;
        case SCALING_MODE_INTEGER:
        case SCALING_MODE_REAL:
        case SCALING_MODE_FIT:
        case SCALING_MODE_CUSTOM:
            vi_hook_set_scaling_mode(VI_SCALING_MODE_EXACT);
            vi_hook_set_layer_crop(0, 0, (int32_t)g_config.src_width, (int32_t)g_config.src_height);
            vi_hook_set_layer_position((float)g_config.calculated_viewport.x, (float)g_config.calculated_viewport.y);
            vi_hook_set_layer_size(g_config.calculated_viewport.width, g_config.calculated_viewport.height);
            break;
    }
}

void sharpscale_init(void) {
    if (g_initialized) return;

    config_load_defaults(&g_config);
    config_load_global(&g_config);

    vi_hook_init();
    nvn_hook_init();

    g_initialized = true;
    sharpscale_apply_settings();
}

void sharpscale_exit(void) {
    if (!g_initialized) return;

    nvn_hook_exit();
    vi_hook_exit();
    g_initialized = false;
}

/**
 * SaltyNX Entry Point
 */
__attribute__((visibility("default")))
void saltyn_main(void) {
    sharpscale_init();
}
