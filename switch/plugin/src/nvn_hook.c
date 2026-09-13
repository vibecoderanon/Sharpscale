#include "nvn_hook.h"
#include "scaler.h"

/* Weak SaltySD Core symbols for dynamic interception and logging */
extern void* SaltySDCore_FindSymbol(const char* name) __attribute__((weak));
extern void SaltySDCore_ReplaceImport(const char* name, void* new_func) __attribute__((weak));
extern void SaltySDCore_printf(const char* format, ...) __attribute__((weak));

typedef void* (*PFN_nvnBootstrapLoader)(const char* name);

static NvnHookState g_nvn_state = {0};
static PFN_nvnBootstrapLoader orig_nvnBootstrapLoader = NULL;

static inline int fast_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void nvn_hook_window_set_crop(void* window, int x, int y, int w, int h) {
    SharpscaleConfig* cfg = sharpscale_get_config();

    /* Record engine-requested source render dimensions */
    if (w > 0 && h > 0) {
        if (cfg->src_width != (uint32_t)w || cfg->src_height != (uint32_t)h) {
            uint32_t dst_w = cfg->is_docked ? 1920 : 1280;
            uint32_t dst_h = cfg->is_docked ? 1080 : 720;
            sharpscale_update_viewport((uint32_t)w, (uint32_t)h, dst_w, dst_h);
        }
    }

    if (g_nvn_state.orig_nvnWindowSetCrop) {
        if (cfg->scaling_mode != SCALING_MODE_ORIGINAL && cfg->calculated_viewport.width > 0) {
            g_nvn_state.orig_nvnWindowSetCrop(
                window,
                (int)cfg->calculated_viewport.x,
                (int)cfg->calculated_viewport.y,
                (int)cfg->calculated_viewport.width,
                (int)cfg->calculated_viewport.height
            );
        } else {
            g_nvn_state.orig_nvnWindowSetCrop(window, x, y, w, h);
        }
    }
}

void nvn_hook_queue_present_texture(void* queue, void* window, int texture_idx) {
    g_nvn_state.nvn_queue = queue;
    g_nvn_state.nvn_window = window;

    /* Check for live settings update from Tesla Overlay via SharedMemory */
    sharpscale_check_live_updates();

    SharpscaleConfig* cfg = sharpscale_get_config();

    /* Enforce viewport crop when custom scaling is active */
    if (window && g_nvn_state.orig_nvnWindowSetCrop) {
        if (cfg->scaling_mode != SCALING_MODE_ORIGINAL && cfg->calculated_viewport.width > 0) {
            g_nvn_state.orig_nvnWindowSetCrop(
                window,
                (int)cfg->calculated_viewport.x,
                (int)cfg->calculated_viewport.y,
                (int)cfg->calculated_viewport.width,
                (int)cfg->calculated_viewport.height
            );
        }
    }

    if (g_nvn_state.orig_nvnQueuePresentTexture) {
        g_nvn_state.orig_nvnQueuePresentTexture(queue, window, texture_idx);
    }
}

void* nvn_hook_device_get_proc_address(void* device, const char* name) {
    if (!name) return NULL;
    g_nvn_state.nvn_device = device;

    if (fast_strcmp(name, "nvnQueuePresentTexture") == 0) {
        if (!g_nvn_state.orig_nvnQueuePresentTexture && g_nvn_state.orig_nvnDeviceGetProcAddress) {
            g_nvn_state.orig_nvnQueuePresentTexture = (PFN_nvnQueuePresentTexture)g_nvn_state.orig_nvnDeviceGetProcAddress(device, name);
        }
        if (!g_nvn_state.orig_nvnWindowSetCrop && g_nvn_state.orig_nvnDeviceGetProcAddress) {
            g_nvn_state.orig_nvnWindowSetCrop = (PFN_nvnWindowSetCrop)g_nvn_state.orig_nvnDeviceGetProcAddress(device, "nvnWindowSetCrop");
        }
        return (void*)nvn_hook_queue_present_texture;
    }

    if (fast_strcmp(name, "nvnWindowSetCrop") == 0) {
        if (!g_nvn_state.orig_nvnWindowSetCrop && g_nvn_state.orig_nvnDeviceGetProcAddress) {
            g_nvn_state.orig_nvnWindowSetCrop = (PFN_nvnWindowSetCrop)g_nvn_state.orig_nvnDeviceGetProcAddress(device, name);
        }
        return (void*)nvn_hook_window_set_crop;
    }

    if (g_nvn_state.orig_nvnDeviceGetProcAddress) {
        return g_nvn_state.orig_nvnDeviceGetProcAddress(device, name);
    }

    return NULL;
}

void* nvn_hook_bootstrap_loader(const char* name) {
    void* ptr = orig_nvnBootstrapLoader ? orig_nvnBootstrapLoader(name) : NULL;
    if (name && fast_strcmp(name, "nvnDeviceGetProcAddress") == 0) {
        g_nvn_state.orig_nvnDeviceGetProcAddress = (PFN_nvnDeviceGetProcAddress)ptr;
        return (void*)nvn_hook_device_get_proc_address;
    }
    return ptr;
}

void nvn_hook_apply_scaling(void* window, uint32_t src_w, uint32_t src_h) {
    if (!window) return;

    SharpscaleConfig* cfg = sharpscale_get_config();
    uint32_t dst_w = cfg->is_docked ? 1920 : 1280;
    uint32_t dst_h = cfg->is_docked ? 1080 : 720;

    if (cfg->src_width != src_w || cfg->src_height != src_h) {
        sharpscale_update_viewport(src_w, src_h, dst_w, dst_h);
    }

    if (g_nvn_state.orig_nvnWindowSetCrop && cfg->scaling_mode != SCALING_MODE_ORIGINAL) {
        g_nvn_state.orig_nvnWindowSetCrop(
            window,
            (int)cfg->calculated_viewport.x,
            (int)cfg->calculated_viewport.y,
            (int)cfg->calculated_viewport.width,
            (int)cfg->calculated_viewport.height
        );
    }
}

void* nvn_hook_get_proc_address(void* device, const char* name) {
    return nvn_hook_device_get_proc_address(device, name);
}

bool nvn_hook_init(void) {
    if (&SaltySDCore_FindSymbol && &SaltySDCore_ReplaceImport) {
        orig_nvnBootstrapLoader = (PFN_nvnBootstrapLoader)SaltySDCore_FindSymbol("nvnBootstrapLoader");
        if (orig_nvnBootstrapLoader) {
            SaltySDCore_ReplaceImport("nvnBootstrapLoader", (void*)nvn_hook_bootstrap_loader);
            if (&SaltySDCore_printf) {
                SaltySDCore_printf("Sharpscale: nvnBootstrapLoader hooked\n");
            }
            g_nvn_state.is_installed = true;
            return true;
        }
    }
    return false;
}

void nvn_hook_exit(void) {
    if (g_nvn_state.is_installed && orig_nvnBootstrapLoader && &SaltySDCore_ReplaceImport) {
        SaltySDCore_ReplaceImport("nvnBootstrapLoader", (void*)orig_nvnBootstrapLoader);
    }
    g_nvn_state.is_installed = false;
}

