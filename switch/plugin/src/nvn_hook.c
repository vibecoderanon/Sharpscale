#include "nvn_hook.h"
#include "scaler.h"
#include "vi_hook.h"

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

void nvn_hook_queue_present_texture(void* queue, void* window, int texture_idx) {
    g_nvn_state.nvn_queue = queue;
    g_nvn_state.nvn_window = window;

    /* Check for live settings update from Tesla Overlay via SharedMemory */
    sharpscale_check_live_updates();

    if (g_nvn_state.orig_nvnQueuePresentTexture) {
        g_nvn_state.orig_nvnQueuePresentTexture(queue, window, texture_idx);
    }
}

void nvn_hook_window_builder_set_textures(void* builder, int numTextures, void** textures) {
    if (g_nvn_state.orig_nvnWindowBuilderSetTextures) {
        g_nvn_state.orig_nvnWindowBuilderSetTextures(builder, numTextures, textures);
    }

    if (numTextures > 0 && textures && textures[0]) {
        uint32_t w = 0, h = 0;
        if (g_nvn_state.orig_nvnTextureGetWidth) {
            w = (uint32_t)g_nvn_state.orig_nvnTextureGetWidth(textures[0]);
        }
        if (g_nvn_state.orig_nvnTextureGetHeight) {
            h = (uint32_t)g_nvn_state.orig_nvnTextureGetHeight(textures[0]);
        }

        if (w > 0 && h > 0) {
            SharpscaleConfig* cfg = sharpscale_get_config();
            cfg->dst_width = w;
            cfg->dst_height = h;
            cfg->is_docked = (w > 1280 || h > 720);
            vi_hook_set_docked(cfg->is_docked);
            sharpscale_apply_settings();

            if (&SaltySDCore_printf) {
                SaltySDCore_printf("Sharpscale: Window swapchain surface: %ux%u (docked=%d)\n", w, h, cfg->is_docked);
            }
        }
    }
}

static int s_last_vp_x = -999, s_last_vp_y = -999, s_last_vp_w = -999, s_last_vp_h = -999;

void nvn_hook_command_buffer_set_viewport(void* cmdBuf, int x, int y, int width, int height) {
    SharpscaleConfig* cfg = sharpscale_get_config();

    if (x != s_last_vp_x || y != s_last_vp_y || width != s_last_vp_w || height != s_last_vp_h) {
        s_last_vp_x = x; s_last_vp_y = y; s_last_vp_w = width; s_last_vp_h = height;
        if (&SaltySDCore_printf) {
            SaltySDCore_printf("Sharpscale: SetViewport(%d, %d, %d, %d)\n", x, y, width, height);
        }
    }

    /* Intercept the main display/presentation viewport pass */
    bool is_main_pass = (width == (int)cfg->dst_width && height == (int)cfg->dst_height) ||
                        (width >= 1280 && height >= 720 && x == 0 && y == 0);

    if (is_main_pass && cfg->scaling_mode != SCALING_MODE_ORIGINAL && cfg->calculated_viewport.width > 0) {
        if (g_nvn_state.orig_nvnCommandBufferSetViewport) {
            g_nvn_state.orig_nvnCommandBufferSetViewport(
                cmdBuf,
                (int)cfg->calculated_viewport.x,
                (int)cfg->calculated_viewport.y,
                (int)cfg->calculated_viewport.width,
                (int)cfg->calculated_viewport.height
            );
        }
        return;
    }

    if (g_nvn_state.orig_nvnCommandBufferSetViewport) {
        g_nvn_state.orig_nvnCommandBufferSetViewport(cmdBuf, x, y, width, height);
    }
}

void nvn_hook_command_buffer_set_scissor(void* cmdBuf, int x, int y, int width, int height) {
    SharpscaleConfig* cfg = sharpscale_get_config();

    bool is_main_pass = (width == (int)cfg->dst_width && height == (int)cfg->dst_height) ||
                        (width >= 1280 && height >= 720 && x == 0 && y == 0);

    if (is_main_pass && cfg->scaling_mode != SCALING_MODE_ORIGINAL && cfg->calculated_viewport.width > 0) {
        if (g_nvn_state.orig_nvnCommandBufferSetScissor) {
            g_nvn_state.orig_nvnCommandBufferSetScissor(
                cmdBuf,
                (int)cfg->calculated_viewport.x,
                (int)cfg->calculated_viewport.y,
                (int)cfg->calculated_viewport.width,
                (int)cfg->calculated_viewport.height
            );
        }
        return;
    }

    if (g_nvn_state.orig_nvnCommandBufferSetScissor) {
        g_nvn_state.orig_nvnCommandBufferSetScissor(cmdBuf, x, y, width, height);
    }
}

void nvn_hook_command_buffer_set_viewports(void* cmdBuf, int start, int count, const void* viewports) {
    SharpscaleConfig* cfg = sharpscale_get_config();

    if (cfg->scaling_mode != SCALING_MODE_ORIGINAL && cfg->calculated_viewport.width > 0 &&
        count > 0 && count <= 16 && viewports) {
        NVNviewport vps[16];
        const NVNviewport* src_vps = (const NVNviewport*)viewports;
        bool modified = false;

        for (int i = 0; i < count; i++) {
            vps[i] = src_vps[i];
            if ((vps[i].width == (float)cfg->dst_width && vps[i].height == (float)cfg->dst_height) ||
                (vps[i].width >= 1280.0f && vps[i].height >= 720.0f && vps[i].x == 0.0f && vps[i].y == 0.0f)) {
                vps[i].x = (float)cfg->calculated_viewport.x;
                vps[i].y = (float)cfg->calculated_viewport.y;
                vps[i].width = (float)cfg->calculated_viewport.width;
                vps[i].height = (float)cfg->calculated_viewport.height;
                modified = true;
            }
        }

        if (modified && g_nvn_state.orig_nvnCommandBufferSetViewports) {
            g_nvn_state.orig_nvnCommandBufferSetViewports(cmdBuf, start, count, vps);
            return;
        }
    }

    if (g_nvn_state.orig_nvnCommandBufferSetViewports) {
        g_nvn_state.orig_nvnCommandBufferSetViewports(cmdBuf, start, count, viewports);
    }
}

void nvn_hook_command_buffer_set_scissors(void* cmdBuf, int start, int count, const void* scissors) {
    SharpscaleConfig* cfg = sharpscale_get_config();

    if (cfg->scaling_mode != SCALING_MODE_ORIGINAL && cfg->calculated_viewport.width > 0 &&
        count > 0 && count <= 16 && scissors) {
        NVNscissor scs[16];
        const NVNscissor* src_scs = (const NVNscissor*)scissors;
        bool modified = false;

        for (int i = 0; i < count; i++) {
            scs[i] = src_scs[i];
            if ((scs[i].width == (int)cfg->dst_width && scs[i].height == (int)cfg->dst_height) ||
                (scs[i].width >= 1280 && scs[i].height >= 720 && scs[i].x == 0 && scs[i].y == 0)) {
                scs[i].x = (int)cfg->calculated_viewport.x;
                scs[i].y = (int)cfg->calculated_viewport.y;
                scs[i].width = (int)cfg->calculated_viewport.width;
                scs[i].height = (int)cfg->calculated_viewport.height;
                modified = true;
            }
        }

        if (modified && g_nvn_state.orig_nvnCommandBufferSetScissors) {
            g_nvn_state.orig_nvnCommandBufferSetScissors(cmdBuf, start, count, scs);
            return;
        }
    }

    if (g_nvn_state.orig_nvnCommandBufferSetScissors) {
        g_nvn_state.orig_nvnCommandBufferSetScissors(cmdBuf, start, count, scissors);
    }
}

void* nvn_hook_device_get_proc_address(void* device, const char* name) {
    if (!name) return NULL;
    g_nvn_state.nvn_device = device;

    if (fast_strcmp(name, "nvnQueuePresentTexture") == 0) {
        if (!g_nvn_state.orig_nvnQueuePresentTexture && g_nvn_state.orig_nvnDeviceGetProcAddress) {
            g_nvn_state.orig_nvnQueuePresentTexture = (PFN_nvnQueuePresentTexture)g_nvn_state.orig_nvnDeviceGetProcAddress(device, name);
        }
        return (void*)nvn_hook_queue_present_texture;
    }

    if (fast_strcmp(name, "nvnWindowBuilderSetTextures") == 0) {
        if (!g_nvn_state.orig_nvnWindowBuilderSetTextures && g_nvn_state.orig_nvnDeviceGetProcAddress) {
            g_nvn_state.orig_nvnWindowBuilderSetTextures = (PFN_nvnWindowBuilderSetTextures)g_nvn_state.orig_nvnDeviceGetProcAddress(device, name);
        }
        return (void*)nvn_hook_window_builder_set_textures;
    }

    if (fast_strcmp(name, "nvnCommandBufferSetViewport") == 0) {
        if (!g_nvn_state.orig_nvnCommandBufferSetViewport && g_nvn_state.orig_nvnDeviceGetProcAddress) {
            g_nvn_state.orig_nvnCommandBufferSetViewport = (PFN_nvnCommandBufferSetViewport)g_nvn_state.orig_nvnDeviceGetProcAddress(device, name);
        }
        return (void*)nvn_hook_command_buffer_set_viewport;
    }

    if (fast_strcmp(name, "nvnCommandBufferSetViewports") == 0) {
        if (!g_nvn_state.orig_nvnCommandBufferSetViewports && g_nvn_state.orig_nvnDeviceGetProcAddress) {
            g_nvn_state.orig_nvnCommandBufferSetViewports = (PFN_nvnCommandBufferSetViewports)g_nvn_state.orig_nvnDeviceGetProcAddress(device, name);
        }
        return (void*)nvn_hook_command_buffer_set_viewports;
    }

    if (fast_strcmp(name, "nvnCommandBufferSetScissor") == 0) {
        if (!g_nvn_state.orig_nvnCommandBufferSetScissor && g_nvn_state.orig_nvnDeviceGetProcAddress) {
            g_nvn_state.orig_nvnCommandBufferSetScissor = (PFN_nvnCommandBufferSetScissor)g_nvn_state.orig_nvnDeviceGetProcAddress(device, name);
        }
        return (void*)nvn_hook_command_buffer_set_scissor;
    }

    if (fast_strcmp(name, "nvnCommandBufferSetScissors") == 0) {
        if (!g_nvn_state.orig_nvnCommandBufferSetScissors && g_nvn_state.orig_nvnDeviceGetProcAddress) {
            g_nvn_state.orig_nvnCommandBufferSetScissors = (PFN_nvnCommandBufferSetScissors)g_nvn_state.orig_nvnDeviceGetProcAddress(device, name);
        }
        return (void*)nvn_hook_command_buffer_set_scissors;
    }

    if (fast_strcmp(name, "nvnTextureGetWidth") == 0) {
        if (!g_nvn_state.orig_nvnTextureGetWidth && g_nvn_state.orig_nvnDeviceGetProcAddress) {
            g_nvn_state.orig_nvnTextureGetWidth = (PFN_nvnTextureGetWidth)g_nvn_state.orig_nvnDeviceGetProcAddress(device, name);
        }
        return (void*)g_nvn_state.orig_nvnTextureGetWidth;
    }

    if (fast_strcmp(name, "nvnTextureGetHeight") == 0) {
        if (!g_nvn_state.orig_nvnTextureGetHeight && g_nvn_state.orig_nvnDeviceGetProcAddress) {
            g_nvn_state.orig_nvnTextureGetHeight = (PFN_nvnTextureGetHeight)g_nvn_state.orig_nvnDeviceGetProcAddress(device, name);
        }
        return (void*)g_nvn_state.orig_nvnTextureGetHeight;
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

