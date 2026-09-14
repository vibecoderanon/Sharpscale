#include "sharpscale_nx.h"
#include "nvn_hook.h"
#include "vi_hook.h"
#include "scaler.h"
#include "config.h"
#include <stddef.h>

static SharpscaleConfig g_config;
static bool g_initialized = false;
static SharpscaleSharedMemory* g_shmem = NULL;
static uint32_t g_last_seq = 0;

/* SaltySD dynamic symbols exported by saltynx_core.elf */
extern uint64_t SaltySD_CheckIfSharedMemoryAvailable(ptrdiff_t *offset, uint64_t size) __attribute__((weak));
extern uint64_t SaltySD_GetSharedMemoryHandle(uint32_t *retrieve) __attribute__((weak));
extern void SaltySDCore_printf(const char* format, ...) __attribute__((weak));

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
/* Raw SVC functions implemented in crt0.s */
extern uint32_t raw_svcGetInfo(uint64_t* out, uint32_t id0, uint64_t handle, uint64_t id1);
extern uint32_t raw_svcQueryMemory(SwitchMemoryInfo* info, uint32_t* page_info, uint64_t address);
extern uint32_t raw_svcMapSharedMemory(uint32_t handle, void* address, size_t size, uint32_t permission);
extern uint32_t raw_svcUnmapSharedMemory(uint32_t handle, void* address, size_t size);


static inline uint64_t get_current_title_id(void) {
    uint64_t title_id = 0;
    raw_svcGetInfo(&title_id, 18 /* InfoType_ProgramId */, 0xFFFF8001ULL, 0);
    return title_id;
}

static void sharpscale_apply_title_profile(uint64_t title_id) {
    g_config.title_id = title_id;

    switch (title_id) {
        case 0x0100C62011050000ULL: /* NSO Game Boy Advance */
            g_config.src_width = 240;
            g_config.src_height = 160;
            g_config.aspect_ratio = ASPECT_RATIO_3_2;
            break;
        case 0x010012F017576000ULL: /* NSO Game Boy / Game Boy Color */
            g_config.src_width = 160;
            g_config.src_height = 144;
            g_config.aspect_ratio = ASPECT_RATIO_10_9;
            break;
        case 0x0100D870045B6000ULL: /* NSO NES / Famicom */
            g_config.src_width = 256;
            g_config.src_height = 240;
            g_config.aspect_ratio = ASPECT_RATIO_4_3;
            break;
        case 0x01008D300C50C000ULL: /* NSO Super NES / Super Famicom */
            g_config.src_width = 256;
            g_config.src_height = 224;
            g_config.aspect_ratio = ASPECT_RATIO_4_3;
            break;
        case 0x0100C9A00ECE6000ULL: /* NSO Nintendo 64 */
            g_config.src_width = 320;
            g_config.src_height = 240;
            g_config.aspect_ratio = ASPECT_RATIO_4_3;
            break;
        case 0x01006BB00C6F0000ULL: /* NSO Sega Genesis / Mega Drive */
            g_config.src_width = 320;
            g_config.src_height = 224;
            g_config.aspect_ratio = ASPECT_RATIO_4_3;
            break;
        default:
            break;
    }
}

static void* find_free_aslr_address(size_t size) {
    uint64_t aslr_base = 0, aslr_size = 0;
    uint32_t rc1 = raw_svcGetInfo(&aslr_base, 12 /* InfoType_AslrRegionAddress */, 0xFFFF8001ULL, 0);
    uint32_t rc2 = raw_svcGetInfo(&aslr_size, 13 /* InfoType_AslrRegionSize */, 0xFFFF8001ULL, 0);

    if (rc1 != 0 || aslr_base == 0) {
        aslr_base = 0x8000000ULL;
    }
    if (rc2 != 0 || aslr_size == 0) {
        aslr_size = 0x1000000000ULL; /* 64GB default ASLR space */
    }

    uint64_t alias_base = 0, alias_size = 0;
    raw_svcGetInfo(&alias_base, 2 /* InfoType_AliasRegionAddress */, 0xFFFF8001ULL, 0);
    raw_svcGetInfo(&alias_size, 3 /* InfoType_AliasRegionSize */, 0xFFFF8001ULL, 0);

    uint64_t heap_base = 0, heap_size = 0;
    raw_svcGetInfo(&heap_base, 4 /* InfoType_HeapRegionAddress */, 0xFFFF8001ULL, 0);
    raw_svcGetInfo(&heap_size, 5 /* InfoType_HeapRegionSize */, 0xFFFF8001ULL, 0);

    if (&SaltySDCore_printf) {
        SaltySDCore_printf("Sharpscale: aslr=0x%lx+0x%lx, alias=0x%lx+0x%lx, heap=0x%lx+0x%lx, rc1=0x%x, rc2=0x%x\n",
            aslr_base, aslr_size, alias_base, alias_size, heap_base, heap_size, rc1, rc2);
    }

    /* Scan for unmapped page inside ASLR region */
    uint64_t addr = aslr_base + 0x20000000ULL; /* 512MB into ASLR to avoid lower mappings */
    SwitchMemoryInfo minfo;
    uint32_t pinfo = 0;

    for (int attempts = 0; attempts < 512 && addr + size < aslr_base + aslr_size; attempts++) {
        /* Avoid alias region */
        if (alias_size > 0 && addr < alias_base + alias_size && addr + size > alias_base) {
            addr = alias_base + alias_size + 0x1000;
            continue;
        }
        /* Avoid heap region */
        if (heap_size > 0 && addr < heap_base + heap_size && addr + size > heap_base) {
            addr = heap_base + heap_size + 0x1000;
            continue;
        }

        uint32_t qrc = raw_svcQueryMemory(&minfo, &pinfo, addr);
        if (qrc != 0) {
            addr += 0x200000ULL;
            continue;
        }

        if (minfo.type == 0 && minfo.size >= size) {
            uint64_t candidate = (addr + 0xFFFULL) & ~0xFFFULL;
            if (candidate < aslr_base) candidate = aslr_base;
            if (candidate + size <= minfo.base_addr + minfo.size && candidate + size <= aslr_base + aslr_size) {
                return (void*)candidate;
            }
        }

        if (minfo.size == 0) {
            addr += 0x200000ULL;
        } else {
            addr = minfo.base_addr + minfo.size;
        }
    }
    return NULL;
}


static void sharpscale_init_shmem(void) {
    if (&SaltySD_CheckIfSharedMemoryAvailable && &SaltySD_GetSharedMemoryHandle) {
        ptrdiff_t offset = 0;
        /* Request extra 16 bytes so we can offset past SaltyNX's reserved bytes at 0..3 */
        uint64_t rc = SaltySD_CheckIfSharedMemoryAvailable(&offset, sizeof(SharpscaleSharedMemory) + 16);
        if (rc == 0) {
            uint32_t handle = 0;
            rc = SaltySD_GetSharedMemoryHandle(&handle);
            if (rc == 0 && handle != 0) {
                void* map_addr = find_free_aslr_address(0x1000);
                if (map_addr) {
                    uint32_t map_rc = raw_svcMapSharedMemory(handle, map_addr, 0x1000, 3 /* Perm_Rw */);
                    if (map_rc == 0) {
                        /* SaltyNX uses shmem[0..3] for internal display sync & refresh rate.
                         * Always offset by at least 16 bytes to prevent corruption. */
                        if (offset < 16) {
                            offset = 16;
                        }
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
                        g_shmem->title_id = g_config.title_id;
                        g_shmem->src_width = g_config.src_width;
                        g_shmem->src_height = g_config.src_height;
                        g_shmem->dst_width = g_config.dst_width;
                        g_shmem->dst_height = g_config.dst_height;
                    }
                    if (&SaltySDCore_printf) {
                        SaltySDCore_printf("Sharpscale: shmem map_addr=%p, rc=0x%x, final_offset=%ld\n", map_addr, map_rc, (long)offset);
                    }
                } else if (&SaltySDCore_printf) {
                    SaltySDCore_printf("Sharpscale: find_free_aslr_address failed\n");
                }
            } else if (&SaltySDCore_printf) {
                SaltySDCore_printf("Sharpscale: SaltySD_GetSharedMemoryHandle failed rc=0x%lx\n", rc);
            }
        } else if (&SaltySDCore_printf) {
            SaltySDCore_printf("Sharpscale: SaltySD_CheckIfSharedMemoryAvailable failed rc=0x%lx\n", rc);
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
    if (g_config.dst_width > 0) dst_w = g_config.dst_width;
    if (g_config.dst_height > 0) dst_h = g_config.dst_height;

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
            vi_hook_set_layer_crop(0, 0, (int32_t)dst_w, (int32_t)dst_h);
            vi_hook_set_layer_position((float)g_config.calculated_viewport.x, (float)g_config.calculated_viewport.y);
            vi_hook_set_layer_size(g_config.calculated_viewport.width, g_config.calculated_viewport.height);
            break;
    }
}

void sharpscale_check_live_updates(void) {
    if (!g_initialized) return;

    if (g_shmem && (g_shmem->magic == SHARPSCALE_SHMEM_MAGIC || (g_shmem->magic & 0xFFFF00FF) == (SHARPSCALE_SHMEM_MAGIC & 0xFFFF00FF))) {
        /* Sync Title ID if detected by overlay but not kernel */
        if (g_config.title_id == 0 && g_shmem->title_id != 0) {
            sharpscale_apply_title_profile(g_shmem->title_id);
            config_load_title(g_shmem->title_id, &g_config);
            sharpscale_apply_settings();
            if (&SaltySDCore_printf) {
                SaltySDCore_printf("Sharpscale: synced title_id from overlay: 0x%016lx\n", g_shmem->title_id);
            }
        }

        if (g_shmem->sequence_id != g_last_seq) {
            g_last_seq = g_shmem->sequence_id;
            g_config.scaling_mode = (SharpscaleScalingMode)g_shmem->scaling_mode;
            g_config.filter_type = (SharpscaleFilterType)g_shmem->filter_type;
            g_config.aspect_ratio = (SharpscaleAspectRatio)g_shmem->aspect_ratio;
            g_config.sharpness_strength = g_shmem->sharpness;
            g_config.force_1080p_capture = (g_shmem->force_1080p != 0);
            sharpscale_apply_settings();
            if (&SaltySDCore_printf) {
                SaltySDCore_printf("Sharpscale: live update seq=%u mode=%d vp=(%d,%d,%d,%d)\n",
                    g_last_seq, g_config.scaling_mode,
                    (int)g_config.calculated_viewport.x, (int)g_config.calculated_viewport.y,
                    (int)g_config.calculated_viewport.width, (int)g_config.calculated_viewport.height);
            }
        }

        /* Report telemetry back to overlay */
        g_shmem->is_plugin_alive = 1;
        g_shmem->is_docked = g_config.is_docked ? 1 : 0;
        if (g_shmem->title_id == 0) g_shmem->title_id = g_config.title_id;
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

    uint64_t tid = get_current_title_id();
    if (&SaltySDCore_printf) {
        SaltySDCore_printf("Sharpscale: detected title_id=0x%016lx\n", tid);
    }
    if (tid != 0) {
        sharpscale_apply_title_profile(tid);
        config_load_title(tid, &g_config);
    }

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
