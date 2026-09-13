#include "vi_hook.h"

/* SaltySD dynamic symbols */
extern void* SaltySDCore_FindSymbol(const char* name) __attribute__((weak));

typedef int (*PFN_GetOperationMode)(void);

static bool g_vi_initialized = false;
static ViScalingMode g_current_mode = VI_SCALING_MODE_NONE;
static PFN_GetOperationMode s_pfnGetOperationMode = NULL;

bool vi_hook_init(void) {
    if (&SaltySDCore_FindSymbol) {
        s_pfnGetOperationMode = (PFN_GetOperationMode)SaltySDCore_FindSymbol("_ZN2nn2oe16GetOperationModeEv");
    }
    g_vi_initialized = true;
    return true;
}

void vi_hook_exit(void) {
    g_vi_initialized = false;
    s_pfnGetOperationMode = NULL;
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
    if (s_pfnGetOperationMode) {
        return (s_pfnGetOperationMode() == 1);
    }
    return false;
}

