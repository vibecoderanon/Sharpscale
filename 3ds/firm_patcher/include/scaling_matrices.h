#ifndef SCALING_MATRICES_H
#define SCALING_MATRICES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Filter Matrix Profiles for 3DS Hardware 2D Polyphase Scaler
 * Each row contains 5-tap filter coefficients (signed fixed-point s0.8 / s0.10)
 */

/* 1. Point / Nearest Neighbor (Sharp raw pixels) */
static const int16_t MATRIX_POINT[8][5] = {
    {   0,   0, 256,   0,   0 },
    {   0,   0, 256,   0,   0 },
    {   0,   0, 256,   0,   0 },
    {   0,   0, 256,   0,   0 },
    {   0,   0, 256,   0,   0 },
    {   0,   0, 256,   0,   0 },
    {   0,   0, 256,   0,   0 },
    {   0,   0, 256,   0,   0 }
};

/* 2. Sharp-Bilinear (Crisp edges with anti-shimmer interpolation) */
static const int16_t MATRIX_SHARP_BILINEAR[8][5] = {
    {   0,   0, 256,   0,   0 },
    {   0,   0, 230,  26,   0 },
    {   0,   0, 192,  64,   0 },
    {   0,   0, 140, 116,   0 },
    {   0,   0, 128, 128,   0 },
    {   0,  16, 116, 140,   0 },
    {   0,  64, 192,   0,   0 },
    {   0,  26, 230,   0,   0 }
};

/* 3. Catmull-Rom Bicubic Spline Filter */
static const int16_t MATRIX_BICUBIC[8][5] = {
    {   0, -16, 288, -16,   0 },
    {   0, -20, 268,   8,   0 },
    {   0, -22, 236,  42,   0 },
    {   0, -20, 196,  80,   0 },
    {   0, -16, 144, 128,   0 },
    {   0,  -8,  96, 168,   0 },
    {   0,   0,  48, 208,   0 },
    {   0,   0,  12, 244,   0 }
};

#ifdef __cplusplus
}
#endif

#endif /* SCALING_MATRICES_H */
