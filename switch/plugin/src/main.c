#include "sharpscale_nx.h"
#include "nvn_hook.h"
#include "vi_hook.h"
#include "scaler.h"
#include "config.h"

static SharpscaleConfig g_config;
static bool g_initialized = false;
static SharpscaleSharedMemory* g_shmem = NULL;
static uint32_t g_last_seq = 0;

/* SaltySD dynamic symbols exported by saltynx_core.elf */
extern uint64_t SaltySD_CheckIfSharedMemoryAvailable(ptrdiff_t *offset, uint64_t size) __attribute__((weak));
extern uint64_t SaltySD_GetSharedMemoryHandle(uint32_t *retrieve) __attribute__((weak));

typedef struct {
    uint64_t base_addr;
    uint64_t size;
    uint32_t type;
    uint32_t attr;
    uint32_t perm;
    uint32_t ipc_refcount;
    uint32_t device_refcount;
    uint32_t pad;
} SwitchMemoryInfo;

static inline uint32_t raw_svcQueryMemory(SwitchMemoryInfo* info, uint32_t* page_info, uint64_t address) {
    register uint64_t x0 __asm__("x0") = (uint64_t)info;
    register uint64_t x1 __asm__("x1") = (uint64_t)page_info;
    register uint64_t x2 __asm__("x2") = address;
    __asm__ __volatile__ (
        "svc 0x06"
        : "+r"(x0)
        : "r"(x1), "r"(x2)
        : "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "cc", "memory"
    );
    return (uint32_t)x0;
}

static inline uint32_t raw_svcMapSharedMemory(uint32_t handle, void* address, size_t size, uint32_t permission) {
    register uint64_t x0 __asm__("x0") = (uint64_t)handle;
    register uint64_t x1 __asm__("x1") = (uint64_t)address;
    register uint64_t x2 __asm__("x2") = (uint64_t)size;
    register uint64_t x3 __asm__("x3") = (uint64_t)permission;
    __asm__ __volatile__ (
        "svc 0x13"
        : "+r"(x0)
        : "r"(x1), "r"(x2), "r"(x3)
        : "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "cc", "memory"
    );
    return (uint32_t)x0;
}

static void* find_free_address(size_t size) {
    uint64_t addr = 0x80000000ULL;
    SwitchMemoryInfo minfo;
    uint32_t pinfo = 0;
    while (addr < 0x7FFFFFF000ULL) {
        if (raw_svcQueryMemory(&minfo, &pinfo, addr) != 0) {
            addr += 0x200000;
            continue;
        }
        if (minfo.type == 0 && minfo.size >= size) {
            return (void*)minfo.base_addr;
        }
        if (minfo.size == 0) break;
        addr = minfo.base_addr + minfo.size;
    }
    return NULL;
}

static void sharpscale_init_shmem(void) {
    if (&SaltySD_CheckIfSharedMemoryAvailable && &SaltySD_GetSharedMemoryHandle) {
        ptrdiff_t offset = 0;
        uint64_t rc = SaltySD_CheckIfSharedMemoryAvailable(&offset, sizeof(SharpscaleSharedMemory));
        if (rc == 0) {
            uint32_t handle = 0;
            rc = SaltySD_GetSharedMemoryHandle(&handle);
            if (rc == 0 && handle != 0) {
                void* map_addr = find_free_address(0x1000);
                if (map_addr) {
                    uint32_t map_rc = raw_svcMapSharedMemory(handle, map_addr, 0x1000, 3 /* Perm_Rw */);
                    if (map_rc == 0) {
                        g_shmem = (SharpscaleSharedMemory*)((uint8_t*)map_addr + offset);
                        g_shmem->magic = SHARPSCALE_SHMEM_MAGIC;
                        g_shmem->version = SHARPSCALE_SHMEM_VERSION;
                        g_shmem->sequence_id = 0;
                        g_shmem->scaling_mode = (uint8_t)g_config.scaling_mode;
                        g_shmem->filter_type = (uint8_t)g_config.filter_type;
                        g_shmem->aspect_ratio = (uint8_t)g_config.aspect_ratio;
                        g_shmem->sharpness = g_config.sharpness_strength;
                        g_shmem->force_1080p = g_config.force_1080p_capture ? 1 : 0;
                        g_shmem->show_osd = g_config.show_osd_notification ? 1 : 0;
                        g_shmem->is_plugin_alive = 1;
                        g_shmem->is_docked = g_config.is_docked ? 1 : 0;
                        g_shmem->src_width = g_config.src_width;
                        g_shmem->src_height = g_config.src_height;
                        g_shmem->dst_width = g_config.dst_width;
                        g_shmem->dst_height = g_config.dst_height;
                    }
                }
            }
        }
    }
}

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

void sharpscale_check_live_updates(void) {
    if (!g_initialized) return;

    if (g_shmem && g_shmem->magic == SHARPSCALE_SHMEM_MAGIC) {
        if (g_shmem->sequence_id != g_last_seq) {
            g_last_seq = g_shmem->sequence_id;
            g_config.scaling_mode = (SharpscaleScalingMode)g_shmem->scaling_mode;
            g_config.filter_type = (SharpscaleFilterType)g_shmem->filter_type;
            g_config.aspect_ratio = (SharpscaleAspectRatio)g_shmem->aspect_ratio;
            g_config.sharpness_strength = g_shmem->sharpness;
            g_config.force_1080p_capture = (g_shmem->force_1080p != 0);
            sharpscale_apply_settings();
        }

        /* Report telemetry back to overlay */
        g_shmem->is_plugin_alive = 1;
        g_shmem->is_docked = g_config.is_docked ? 1 : 0;
        g_shmem->src_width = g_config.src_width;
        g_shmem->src_height = g_config.src_height;
        g_shmem->dst_width = g_config.dst_width;
        g_shmem->dst_height = g_config.dst_height;
        g_shmem->vp_x = (uint32_t)g_config.calculated_viewport.x;
        g_shmem->vp_y = (uint32_t)g_config.calculated_viewport.y;
        g_shmem->vp_w = (uint32_t)g_config.calculated_viewport.width;
        g_shmem->vp_h = (uint32_t)g_config.calculated_viewport.height;
    }
}

void sharpscale_init(void) {
    if (g_initialized) return;

    config_load_defaults(&g_config);
    config_load_global(&g_config);

    sharpscale_init_shmem();

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
