#include "bezel.h"
#include <string.h>

void bezel_get_geometry(SharpscaleRetroFormat format, bool is_800px, BezelGeometry* out_geom) {
    if (!out_geom) return;

    uint32_t screen_w = is_800px ? 800 : 400;
    uint32_t screen_h = 240;

    out_geom->width = screen_w;
    out_geom->height = screen_h;

    switch (format) {
        case RETRO_FORMAT_GBA: // 240x160
            out_geom->inner_w = is_800px ? 480 : 240; // 2x horizontal in 800px
            out_geom->inner_h = 160;
            break;

        case RETRO_FORMAT_NDS: // 256x192
            out_geom->inner_w = is_800px ? 512 : 256;
            out_geom->inner_h = 192;
            break;

        case RETRO_FORMAT_GB_GBC: // 160x144
        case RETRO_FORMAT_GAMEGEAR:
            out_geom->inner_w = is_800px ? 320 : 160;
            out_geom->inner_h = 144;
            break;

        case RETRO_FORMAT_NES: // 256x224
        case RETRO_FORMAT_SNES:
            out_geom->inner_w = is_800px ? 512 : 256;
            out_geom->inner_h = 224;
            break;

        case RETRO_FORMAT_AUTO:
        default:
            out_geom->inner_w = screen_w;
            out_geom->inner_h = screen_h;
            break;
    }

    out_geom->inner_x = (screen_w - out_geom->inner_w) / 2;
    out_geom->inner_y = (screen_h - out_geom->inner_h) / 2;
}

void bezel_render_border(uint8_t* fb, uint32_t stride, uint32_t screen_w, uint32_t screen_h, const BezelGeometry* geom, uint32_t border_color) {
    if (!fb || !geom) return;

    uint8_t r = (border_color >> 16) & 0xFF;
    uint8_t g = (border_color >> 8) & 0xFF;
    uint8_t b = border_color & 0xFF;

    for (uint32_t y = 0; y < screen_h; y++) {
        for (uint32_t x = 0; x < screen_w; x++) {
            // Check if outside active inner viewport
            if (x < geom->inner_x || x >= (geom->inner_x + geom->inner_w) ||
                y < geom->inner_y || y >= (geom->inner_y + geom->inner_h)) {
                uint32_t offset = (y * stride + x) * 3;
                fb[offset + 0] = b;
                fb[offset + 1] = g;
                fb[offset + 2] = r;
            }
        }
    }
}
