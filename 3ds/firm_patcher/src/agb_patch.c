#include "firm_patcher.h"
#include "scaling_matrices.h"
#include <string.h>

/* Known magic byte sequences near the AGB_FIRM scaling filter table */
static const uint8_t AGB_TABLE_SIGNATURE[8] = { 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x50, 0x00 };

bool agb_apply_matrix(uint8_t* firm_buffer, size_t firm_size, FilterChoice filter) {
    if (!firm_buffer || firm_size < 1024) return false;

    const int16_t (*chosen_matrix)[5] = NULL;
    switch (filter) {
        case FILTER_CHOICE_POINT:          chosen_matrix = MATRIX_POINT; break;
        case FILTER_CHOICE_SHARP_BILINEAR: chosen_matrix = MATRIX_SHARP_BILINEAR; break;
        case FILTER_CHOICE_BICUBIC:        chosen_matrix = MATRIX_BICUBIC; break;
    }

    if (!chosen_matrix) return false;

    /* Scan buffer for AGB scaling table offset */
    for (size_t i = 0; i < firm_size - sizeof(AGB_TABLE_SIGNATURE); i++) {
        if (memcmp(&firm_buffer[i], AGB_TABLE_SIGNATURE, sizeof(AGB_TABLE_SIGNATURE)) == 0) {
            memcpy(&firm_buffer[i], chosen_matrix, sizeof(int16_t) * 8 * 5);
            return true;
        }
    }

    return false;
}
