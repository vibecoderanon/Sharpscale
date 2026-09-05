#include "scaler.h"
#include <math.h>
#include <stdlib.h>

uint32_t scaler_get_max_integer_scale(uint32_t src_w, uint32_t src_h, uint32_t dst_w, uint32_t dst_h) {
    if (src_w == 0 || src_h == 0) return 1;

    uint32_t scale_x = dst_w / src_w;
    uint32_t scale_y = dst_h / src_h;

    uint32_t min_scale = (scale_x < scale_y) ? scale_x : scale_y;
    return (min_scale > 0) ? min_scale : 1;
}

static void get_target_aspect_ratio(SharpscaleAspectRatio aspect, uint32_t src_w, uint32_t src_h, float* out_aspect) {
    switch (aspect) {
        case ASPECT_RATIO_16_9:
            *out_aspect = 16.0f / 9.0f;
            break;
        case ASPECT_RATIO_4_3:
            *out_aspect = 4.0f / 3.0f;
            break;
        case ASPECT_RATIO_3_2:
            *out_aspect = 3.0f / 2.0f;
            break;
        case ASPECT_RATIO_1_1:
            *out_aspect = 1.0f;
            break;
        case ASPECT_RATIO_10_9:
            *out_aspect = 10.0f / 9.0f;
            break;
        case ASPECT_RATIO_AUTO:
        default:
            if (src_h > 0) {
                *out_aspect = (float)src_w / (float)src_h;
            } else {
                *out_aspect = 16.0f / 9.0f;
            }
            break;
    }
}

void scaler_calculate_viewport(
    uint32_t src_w,
    uint32_t src_h,
    uint32_t dst_w,
    uint32_t dst_h,
    SharpscaleScalingMode mode,
    SharpscaleAspectRatio aspect,
    SharpscaleRect* out_viewport,
    float* out_scale_x,
    float* out_scale_y
) {
    if (!out_viewport) return;
    if (src_w == 0) src_w = 1280;
    if (src_h == 0) src_h = 720;
    if (dst_w == 0) dst_w = 1280;
    if (dst_h == 0) dst_h = 720;

    float target_aspect;
    get_target_aspect_ratio(aspect, src_w, src_h, &target_aspect);

    switch (mode) {
        case SCALING_MODE_ORIGINAL: {
            out_viewport->x = 0;
            out_viewport->y = 0;
            out_viewport->width = (int32_t)dst_w;
            out_viewport->height = (int32_t)dst_h;
            if (out_scale_x) *out_scale_x = (float)dst_w / (float)src_w;
            if (out_scale_y) *out_scale_y = (float)dst_h / (float)src_h;
            break;
        }

        case SCALING_MODE_REAL: {
            /* 1:1 Pixel Mapping: Center native resolution */
            int32_t vp_w = (int32_t)src_w;
            int32_t vp_h = (int32_t)src_h;

            out_viewport->width = vp_w;
            out_viewport->height = vp_h;
            out_viewport->x = ((int32_t)dst_w - vp_w) / 2;
            out_viewport->y = ((int32_t)dst_h - vp_h) / 2;

            if (out_scale_x) *out_scale_x = 1.0f;
            if (out_scale_y) *out_scale_y = 1.0f;
            break;
        }

        case SCALING_MODE_INTEGER: {
            /* Maximum integer factor fit */
            uint32_t int_scale = scaler_get_max_integer_scale(src_w, src_h, dst_w, dst_h);
            int32_t vp_w = (int32_t)(src_w * int_scale);
            int32_t vp_h = (int32_t)(src_h * int_scale);

            out_viewport->width = vp_w;
            out_viewport->height = vp_h;
            out_viewport->x = ((int32_t)dst_w - vp_w) / 2;
            out_viewport->y = ((int32_t)dst_h - vp_h) / 2;

            if (out_scale_x) *out_scale_x = (float)int_scale;
            if (out_scale_y) *out_scale_y = (float)int_scale;
            break;
        }

        case SCALING_MODE_FIT: {
            /* Fit to screen while preserving target aspect ratio */
            float display_aspect = (float)dst_w / (float)dst_h;
            int32_t vp_w, vp_h;

            if (display_aspect > target_aspect) {
                /* Pillarboxed (vertical fit) */
                vp_h = (int32_t)dst_h;
                vp_w = (int32_t)roundf((float)dst_h * target_aspect);
            } else {
                /* Letterboxed (horizontal fit) */
                vp_w = (int32_t)dst_w;
                vp_h = (int32_t)roundf((float)dst_w / target_aspect);
            }

            out_viewport->width = vp_w;
            out_viewport->height = vp_h;
            out_viewport->x = ((int32_t)dst_w - vp_w) / 2;
            out_viewport->y = ((int32_t)dst_h - vp_h) / 2;

            if (out_scale_x) *out_scale_x = (float)vp_w / (float)src_w;
            if (out_scale_y) *out_scale_y = (float)vp_h / (float)src_h;
            break;
        }

        case SCALING_MODE_CUSTOM:
        default:
            /* Preserved or modified through explicit configuration */
            break;
    }
}

void scaler_setup_cas_constants(float sharpness, uint32_t src_w, uint32_t src_h, CasFilterConstants* out_constants) {
    if (!out_constants) return;

    /* Clamp sharpness between 0.0 (subtle) and 1.0 (maximum crispness) */
    if (sharpness < 0.0f) sharpness = 0.0f;
    if (sharpness > 1.0f) sharpness = 1.0f;

    /* AMD CAS peak sharpness mapping */
    float sharp = -1.0f / (8.0f - (sharpness * 3.0f));

    /* const0: Render resolution and subpixel scaling offsets */
    out_constants->const0[0] = (float)src_w;
    out_constants->const0[1] = (float)src_h;
    out_constants->const0[2] = 1.0f / (float)src_w;
    out_constants->const0[3] = 1.0f / (float)src_h;

    /* const1: Filter sharpness weights and limit coefficients */
    out_constants->const1[0] = sharp;
    out_constants->const1[1] = sharp;
    out_constants->const1[2] = 8.0f * sharp + 1.0f;
    out_constants->const1[3] = 0.0f;
}
