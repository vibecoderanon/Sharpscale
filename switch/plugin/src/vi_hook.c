#include "vi_hook.h"
#include <stdio.h>
#include <string.h>

static bool g_vi_initialized = false;
static ViScalingMode g_current_mode = VI_SCALING_MODE_NONE;
static bool g_is_docked = false;

bool vi_hook_init(void) {
    g_vi_initialized = true;
    return true;
}

void vi_hook_exit(void) {
    g_vi_initialized = false;
}

void vi_hook_set_scaling_mode(ViScalingMode mode) {
    g_current_mode = mode;
    /* In actual SaltyNX / libnx environment, this invokes:
     * viSetLayerScalingMode(layer, (u32)mode) on the active application display layer.
     */
}

void vi_hook_set_layer_crop(int32_t left, int32_t top, int32_t right, int32_t bottom) {
    (void)left; (void)top; (void)right; (void)bottom;
    /* Sets the source crop rectangle on the VI compositor layer */
}

void vi_hook_set_layer_position(float x, float y) {
    (void)x; (void)y;
    /* Repositions the layer within the output display frame buffer (centering / letterboxing) */
}

void vi_hook_set_layer_size(int32_t width, int32_t height) {
    (void)width; (void)height;
    /* Sets the layer destination size */
}

bool vi_hook_is_docked(void) {
    /* Queries appletGetOperationMode / oeGetPerformanceMode */
    return g_is_docked;
}
