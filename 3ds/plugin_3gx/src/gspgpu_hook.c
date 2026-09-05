#include "gspgpu_hook.h"
#include <stdio.h>
#include <string.h>

static bool g_gspgpu_active = false;
static GspGpuFbInfo g_fb_info = {0};

bool gspgpu_hook_init(void) {
    g_gspgpu_active = true;
    return true;
}

void gspgpu_hook_exit(void) {
    g_gspgpu_active = false;
}

void gspgpu_hook_trigger_flush(void) {
    if (!g_gspgpu_active) return;
    /* Triggers GSPGPU:SetBufferSwap / GSPGPU:FlushDataCache on CTR */
}

void gspgpu_hook_set_custom_fb(uint32_t screen_id, uint32_t paddr, uint32_t stride, uint32_t format) {
    if (screen_id == 0) {
        g_fb_info.top_left_paddr = paddr;
        g_fb_info.top_stride = stride;
        g_fb_info.top_format = format;
    } else {
        g_fb_info.bottom_paddr = paddr;
        g_fb_info.bottom_stride = stride;
        g_fb_info.bottom_format = format;
    }
}
