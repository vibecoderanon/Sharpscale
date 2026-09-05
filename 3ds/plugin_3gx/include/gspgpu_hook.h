#ifndef SHARPSCALE_GSPGPU_HOOK_H
#define SHARPSCALE_GSPGPU_HOOK_H

#include <stdint.h>
#include <stdbool.h>
#include "sharpscale_3ds.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * GSPGPU Framebuffer info on CTR (Nintendo 3DS)
 */
typedef struct {
    uint32_t active_fb;
    uint32_t top_left_paddr;
    uint32_t top_right_paddr;
    uint32_t top_stride;
    uint32_t top_format;
    uint32_t bottom_paddr;
    uint32_t bottom_stride;
    uint32_t bottom_format;
    uint32_t flags;
} GspGpuFbInfo;

bool gspgpu_hook_init(void);
void gspgpu_hook_exit(void);
void gspgpu_hook_trigger_flush(void);
void gspgpu_hook_set_custom_fb(uint32_t screen_id, uint32_t paddr, uint32_t stride, uint32_t format);

#ifdef __cplusplus
}
#endif

#endif /* SHARPSCALE_GSPGPU_HOOK_H */
