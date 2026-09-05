#ifndef SHARPSCALE_VI_HOOK_H
#define SHARPSCALE_VI_HOOK_H

#include <stdint.h>
#include <stdbool.h>
#include "sharpscale_nx.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Vi Scaling Modes in Nintendo Switch Horizon OS
 */
typedef enum {
    VI_SCALING_MODE_NONE                  = 0,
    VI_SCALING_MODE_EXACT                 = 1,
    VI_SCALING_MODE_FIT_TO_LAYER          = 2,
    VI_SCALING_MODE_SCALE_AND_CROP        = 3,
    VI_SCALING_MODE_PRESERVE_ASPECT_RATIO = 4
} ViScalingMode;

/**
 * Vi hook functions to control Horizon OS compositor scaling
 */
bool vi_hook_init(void);
void vi_hook_exit(void);
void vi_hook_set_scaling_mode(ViScalingMode mode);
void vi_hook_set_layer_crop(int32_t left, int32_t top, int32_t right, int32_t bottom);
void vi_hook_set_layer_position(float x, float y);
void vi_hook_set_layer_size(int32_t width, int32_t height);
bool vi_hook_is_docked(void);

#ifdef __cplusplus
}
#endif

#endif /* SHARPSCALE_VI_HOOK_H */
