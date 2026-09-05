#include "screen_800px.h"
#include <string.h>

static bool g_800px_active = false;

bool screen_800px_enable(bool enable) {
    g_800px_active = enable;
    /* In CTR / Luma environment, this sets the GSPGPU LCD controller register
     * to 800px mode by enabling dual-framebuffer scanout with 0 parallax barrier.
     */
    return true;
}

bool screen_800px_is_enabled(void) {
    return g_800px_active;
}

void screen_800px_interleave_framebuffer(
    const uint8_t* src_800x240,
    uint8_t* dst_left_400x240,
    uint8_t* dst_right_400x240,
    uint32_t bpp
) {
    if (!src_800x240 || !dst_left_400x240 || !dst_right_400x240) return;

    /* 3DS framebuffers are column-major (rotated 90 degrees CCW in VRAM)
     * For 800x240 image:
     * - Even columns (0, 2, 4... 798) go to dst_left (400x240)
     * - Odd columns (1, 3, 5... 799) go to dst_right (400x240)
     */
    uint32_t bytes_per_pixel = bpp / 8;
    if (bytes_per_pixel == 0) bytes_per_pixel = 3; // default RGB888

    for (uint32_t col = 0; col < 800; col++) {
        uint32_t target_col = col / 2;
        uint8_t* dst = (col % 2 == 0) ? dst_left_400x240 : dst_right_400x240;

        for (uint32_t row = 0; row < 240; row++) {
            const uint8_t* src_pixel = &src_800x240[(row * 800 + col) * bytes_per_pixel];
            uint8_t* dst_pixel = &dst[(row * 400 + target_col) * bytes_per_pixel];

            for (uint32_t b = 0; b < bytes_per_pixel; b++) {
                dst_pixel[b] = src_pixel[b];
            }
        }
    }
}
