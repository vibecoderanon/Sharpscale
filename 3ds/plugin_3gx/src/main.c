#include "sharpscale_3ds.h"
#include "gspgpu_hook.h"
#include "screen_800px.h"
#include "bezel.h"
#include <string.h>

static Sharpscale3DSConfig g_config;
static bool g_initialized = false;

Sharpscale3DSConfig* sharpscale_3ds_get_config(void) {
    return &g_config;
}

void sharpscale_3ds_apply_config(void) {
    if (!g_initialized) return;

    if (g_config.top_screen_mode == MODE_3DS_HIRES_800PX) {
        screen_800px_enable(true);
    } else {
        screen_800px_enable(false);
    }

    gspgpu_hook_trigger_flush();
}

void sharpscale_3ds_init(void) {
    if (g_initialized) return;

    memset(&g_config, 0, sizeof(Sharpscale3DSConfig));
    g_config.top_screen_mode = MODE_3DS_STANDARD_400PX;
    g_config.filter_mode = FILTER_3DS_CRISP_BILINEAR;
    g_config.retro_format = RETRO_FORMAT_AUTO;
    g_config.enable_custom_bezel = true;
    g_config.bg_color_rgb888 = 0x1A1A1A;

    gspgpu_hook_init();
    g_initialized = true;
    sharpscale_3ds_apply_config();
}

void sharpscale_3ds_exit(void) {
    if (!g_initialized) return;

    screen_800px_enable(false);
    gspgpu_hook_exit();
    g_initialized = false;
}

/**
 * 3GX Plugin Entry Point
 */
__attribute__((visibility("default")))
int main(void) {
    sharpscale_3ds_init();
    return 0;
}
