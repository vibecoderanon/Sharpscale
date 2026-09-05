#ifndef SHARPSCALE_SCREEN_800PX_H
#define SHARPSCALE_SCREEN_800PX_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 800px Mode Driver for 3DS Top Screen
 * Reconfigures the LCD controller to drive left and right eye columns independently
 * as an 800x240 progressive 2D display.
 */
bool screen_800px_enable(bool enable);
bool screen_800px_is_enabled(void);
void screen_800px_interleave_framebuffer(
    const uint8_t* src_800x240,
    uint8_t* dst_left_400x240,
    uint8_t* dst_right_400x240,
    uint32_t bpp
);

#ifdef __cplusplus
}
#endif

#endif /* SHARPSCALE_SCREEN_800PX_H */
