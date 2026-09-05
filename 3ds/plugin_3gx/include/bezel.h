#ifndef SHARPSCALE_BEZEL_H
#define SHARPSCALE_BEZEL_H

#include <stdint.h>
#include <stdbool.h>
#include "sharpscale_3ds.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t inner_x;
    uint32_t inner_y;
    uint32_t inner_w;
    uint32_t inner_h;
} BezelGeometry;

void bezel_get_geometry(SharpscaleRetroFormat format, bool is_800px, BezelGeometry* out_geom);
void bezel_render_border(uint8_t* fb, uint32_t stride, uint32_t screen_w, uint32_t screen_h, const BezelGeometry* geom, uint32_t border_color);

#ifdef __cplusplus
}
#endif

#endif /* SHARPSCALE_BEZEL_H */
