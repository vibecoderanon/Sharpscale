#include "vi_hook.h"

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
}

void vi_hook_set_layer_crop(int32_t left, int32_t top, int32_t right, int32_t bottom) {
    (void)left; (void)top; (void)right; (void)bottom;
}

void vi_hook_set_layer_position(float x, float y) {
    (void)x; (void)y;
}

void vi_hook_set_layer_size(int32_t width, int32_t height) {
    (void)width; (void)height;
}

bool vi_hook_is_docked(void) {
    return g_is_docked;
}

void vi_hook_set_docked(bool docked) {
    g_is_docked = docked;
}



