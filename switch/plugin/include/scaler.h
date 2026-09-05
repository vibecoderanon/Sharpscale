#ifndef SHARPSCALE_SCALER_H
#define SHARPSCALE_SCALER_H

#include <stdint.h>
#include <stdbool.h>
#include "sharpscale_nx.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Calculates optimal viewport, integer scaling multiplier, and letterboxing borders
 */
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
);

/**
 * Calculates Contrast Adaptive Sharpening (CAS) filter constants
 * based on AMD FidelityFX CAS algorithm
 */
typedef struct {
    float const0[4];
    float const1[4];
} CasFilterConstants;

void scaler_setup_cas_constants(float sharpness, uint32_t src_w, uint32_t src_h, CasFilterConstants* out_constants);

/**
 * Computes integer scale factor (e.g. 1x, 2x, 3x) given source and destination dimensions
 */
uint32_t scaler_get_max_integer_scale(uint32_t src_w, uint32_t src_h, uint32_t dst_w, uint32_t dst_h);

#ifdef __cplusplus
}
#endif

#endif /* SHARPSCALE_SCALER_H */
