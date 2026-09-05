#include "nvn_hook.h"
#include "scaler.h"
#include <stdio.h>
#include <string.h>

static NvnHookState g_nvn_state = {0};

void nvn_hook_apply_scaling(void* window, uint32_t src_w, uint32_t src_h) {
    if (!window) return;

    SharpscaleConfig* cfg = sharpscale_get_config();
    uint32_t dst_w = cfg->is_docked ? 1920 : 1280;
    uint32_t dst_h = cfg->is_docked ? 1080 : 720;

    if (cfg->src_width != src_w || cfg->src_height != src_h) {
        sharpscale_update_viewport(src_w, src_h, dst_w, dst_h);
    }

    if (g_nvn_state.orig_nvnWindowSetCrop && cfg->scaling_mode != SCALING_MODE_ORIGINAL) {
        /* Crop to calculated viewport */
        g_nvn_state.orig_nvnWindowSetCrop(
            window,
            0,
            0,
            (int)cfg->src_width,
            (int)cfg->src_height
        );
    }
}

void nvn_hook_queue_present_texture(void* queue, void* window, int texture_idx) {
    g_nvn_state.nvn_queue = queue;
    g_nvn_state.nvn_window = window;

    /* If CAS sharpening is active, execute compute/graphics dispatch pass here */
    SharpscaleConfig* cfg = sharpscale_get_config();
    if (cfg->filter_type == FILTER_TYPE_CAS) {
        CasFilterConstants cas_constants;
        scaler_setup_cas_constants(
            (float)cfg->sharpness_strength / 100.0f,
            cfg->src_width,
            cfg->src_height,
            &cas_constants
        );
    }

    if (g_nvn_state.orig_nvnQueuePresentTexture) {
        g_nvn_state.orig_nvnQueuePresentTexture(queue, window, texture_idx);
    }
}

void* nvn_hook_get_proc_address(void* device, const char* name) {
    if (!name) return NULL;

    if (strcmp(name, "nvnQueuePresentTexture") == 0) {
        if (!g_nvn_state.orig_nvnQueuePresentTexture && g_nvn_state.orig_nvnDeviceGetProcAddress) {
            g_nvn_state.orig_nvnQueuePresentTexture = (PFN_nvnQueuePresentTexture)g_nvn_state.orig_nvnDeviceGetProcAddress(device, name);
        }
        return (void*)nvn_hook_queue_present_texture;
    }

    if (strcmp(name, "nvnWindowSetCrop") == 0) {
        if (!g_nvn_state.orig_nvnWindowSetCrop && g_nvn_state.orig_nvnDeviceGetProcAddress) {
            g_nvn_state.orig_nvnWindowSetCrop = (PFN_nvnWindowSetCrop)g_nvn_state.orig_nvnDeviceGetProcAddress(device, name);
        }
        return (void*)g_nvn_state.orig_nvnWindowSetCrop;
    }

    if (g_nvn_state.orig_nvnDeviceGetProcAddress) {
        return g_nvn_state.orig_nvnDeviceGetProcAddress(device, name);
    }

    return NULL;
}

bool nvn_hook_init(void) {
    g_nvn_state.is_installed = true;
    return true;
}

void nvn_hook_exit(void) {
    g_nvn_state.is_installed = false;
}
