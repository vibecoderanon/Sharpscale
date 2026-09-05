#ifndef FIRM_PATCHER_H
#define FIRM_PATCHER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FIRM_TARGET_TWL = 0, /**< Nintendo DS Mode (TWL_FIRM) */
    FIRM_TARGET_AGB = 1  /**< Game Boy Advance Mode (AGB_FIRM) */
} FirmTarget;

typedef enum {
    FILTER_CHOICE_POINT          = 0,
    FILTER_CHOICE_SHARP_BILINEAR = 1,
    FILTER_CHOICE_BICUBIC        = 2
} FilterChoice;

bool firm_patch_file(const char* input_firm, const char* output_firm, FirmTarget target, FilterChoice filter);
bool twl_apply_matrix(uint8_t* firm_buffer, size_t firm_size, FilterChoice filter);
bool agb_apply_matrix(uint8_t* firm_buffer, size_t firm_size, FilterChoice filter);

#ifdef __cplusplus
}
#endif

#endif /* FIRM_PATCHER_H */
