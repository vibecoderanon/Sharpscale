#include "vi_hook.h"

/* Weak SaltySD Core symbols for dynamic interception and logging */
extern void* SaltySDCore_FindSymbol(const char* name) __attribute__((weak));
extern void SaltySDCore_ReplaceImport(const char* name, void* new_func) __attribute__((weak));
extern void SaltySDCore_printf(const char* format, ...) __attribute__((weak));

typedef void* (*PFN_CreateLayer)(void* layer, void* display);
typedef void (*PFN_SetLayerScalingMode)(void* layer, int mode);
typedef void (*PFN_SetLayerPosition)(void* layer, float x, float y);
typedef void (*PFN_SetLayerSize)(void* layer, int width, int height);
typedef void (*PFN_SetLayerCrop)(void* layer, int left, int top, int right, int bottom);

static bool g_vi_initialized = false;
static ViScalingMode g_current_mode = VI_SCALING_MODE_NONE;
static bool g_is_docked = false;
static void* g_active_layer = NULL;

static PFN_CreateLayer orig_CreateLayer = NULL;
static PFN_SetLayerScalingMode real_SetLayerScalingMode = NULL;
static PFN_SetLayerPosition real_SetLayerPosition = NULL;
static PFN_SetLayerSize real_SetLayerSize = NULL;
static PFN_SetLayerCrop real_SetLayerCrop = NULL;

void* hook_CreateLayer(void* layer, void* display) {
    g_active_layer = layer;
    if (&SaltySDCore_printf) {
        SaltySDCore_printf("Sharpscale: Captured active VI Layer = %p\n", layer);
    }
    if (orig_CreateLayer) {
        return orig_CreateLayer(layer, display);
    }
    return NULL;
}

bool vi_hook_init(void) {
    g_vi_initialized = true;

    if (&SaltySDCore_FindSymbol) {
        const char* create_syms[] = {
            "_ZN2nn2vi11CreateLayerEPNS0_5LayerEPNS0_7DisplayE",
            "_ZN2nn2vi11CreateLayerEPNS0_7DisplayEPNS0_5LayerE",
            "viCreateLayer",
            NULL
        };
        for (int i = 0; create_syms[i]; i++) {
            void* sym = SaltySDCore_FindSymbol(create_syms[i]);
            if (sym) {
                orig_CreateLayer = (PFN_CreateLayer)sym;
                if (&SaltySDCore_ReplaceImport) {
                    SaltySDCore_ReplaceImport(create_syms[i], (void*)hook_CreateLayer);
                    if (&SaltySDCore_printf) {
                        SaltySDCore_printf("Sharpscale: Hooked %s at %p\n", create_syms[i], sym);
                    }
                }
                break;
            }
        }

        const char* scale_syms[] = {
            "_ZN2nn2vi19SetLayerScalingModeEPNS0_5LayerENS0_11ScalingModeE",
            "_ZN2nn2vi19SetLayerScalingModeEPNS0_5LayerEi",
            "viSetLayerScalingMode",
            NULL
        };
        for (int i = 0; scale_syms[i]; i++) {
            void* sym = SaltySDCore_FindSymbol(scale_syms[i]);
            if (sym) {
                real_SetLayerScalingMode = (PFN_SetLayerScalingMode)sym;
                break;
            }
        }

        const char* pos_syms[] = {
            "_ZN2nn2vi16SetLayerPositionEPNS0_5LayerEff",
            "viSetLayerPosition",
            NULL
        };
        for (int i = 0; pos_syms[i]; i++) {
            void* sym = SaltySDCore_FindSymbol(pos_syms[i]);
            if (sym) {
                real_SetLayerPosition = (PFN_SetLayerPosition)sym;
                break;
            }
        }

        const char* size_syms[] = {
            "_ZN2nn2vi12SetLayerSizeEPNS0_5LayerEii",
            "_ZN2nn2vi12SetLayerSizeEPNS0_5LayerEll",
            "viSetLayerSize",
            NULL
        };
        for (int i = 0; size_syms[i]; i++) {
            void* sym = SaltySDCore_FindSymbol(size_syms[i]);
            if (sym) {
                real_SetLayerSize = (PFN_SetLayerSize)sym;
                break;
            }
        }

        const char* crop_syms[] = {
            "_ZN2nn2vi12SetLayerCropEPNS0_5LayerEiiii",
            NULL
        };
        for (int i = 0; crop_syms[i]; i++) {
            void* sym = SaltySDCore_FindSymbol(crop_syms[i]);
            if (sym) {
                real_SetLayerCrop = (PFN_SetLayerCrop)sym;
                break;
            }
        }
    }

    return true;
}

void vi_hook_exit(void) {
    g_vi_initialized = false;
    g_active_layer = NULL;
}

void vi_hook_set_scaling_mode(ViScalingMode mode) {
    g_current_mode = mode;
    if (g_active_layer && real_SetLayerScalingMode) {
        real_SetLayerScalingMode(g_active_layer, (int)mode);
    }
}

void vi_hook_set_layer_crop(int32_t left, int32_t top, int32_t right, int32_t bottom) {
    if (g_active_layer && real_SetLayerCrop) {
        real_SetLayerCrop(g_active_layer, left, top, right, bottom);
    }
}

void vi_hook_set_layer_position(float x, float y) {
    if (g_active_layer && real_SetLayerPosition) {
        real_SetLayerPosition(g_active_layer, x, y);
    }
}

void vi_hook_set_layer_size(int32_t width, int32_t height) {
    if (g_active_layer && real_SetLayerSize) {
        real_SetLayerSize(g_active_layer, width, height);
    }
}

bool vi_hook_is_docked(void) {
    return g_is_docked;
}

void vi_hook_set_docked(bool docked) {
    g_is_docked = docked;
}


