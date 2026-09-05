#include "firm_patcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool firm_patch_file(const char* input_firm, const char* output_firm, FirmTarget target, FilterChoice filter) {
    FILE* in = fopen(input_firm, "rb");
    if (!in) {
        fprintf(stderr, "Error: Could not open input FIRM: %s\n", input_firm);
        return false;
    }

    fseek(in, 0, SEEK_END);
    long size = ftell(in);
    fseek(in, 0, SEEK_SET);

    if (size <= 0 || size > 16 * 1024 * 1024) {
        fclose(in);
        return false;
    }

    uint8_t* buffer = (uint8_t*)malloc(size);
    if (!buffer) {
        fclose(in);
        return false;
    }

    if (fread(buffer, 1, size, in) != (size_t)size) {
        free(buffer);
        fclose(in);
        return false;
    }
    fclose(in);

    bool patched = false;
    if (target == FIRM_TARGET_TWL) {
        patched = twl_apply_matrix(buffer, size, filter);
    } else {
        patched = agb_apply_matrix(buffer, size, filter);
    }

    if (!patched) {
        fprintf(stderr, "Warning: Scaling table signature not found or patch failed.\n");
    }

    FILE* out = fopen(output_firm, "wb");
    if (!out) {
        free(buffer);
        return false;
    }

    fwrite(buffer, 1, size, out);
    fclose(out);
    free(buffer);

    return patched;
}

int main(int argc, char** argv) {
    printf("Sharpscale-3DS FIRM Patcher v1.0.0\n");

    if (argc < 4) {
        printf("Usage: %s <twl|agb> <point|sharp|bicubic> <input.firm> [output.firm]\n", argv[0]);
        return 1;
    }

    FirmTarget target = (strcmp(argv[1], "agb") == 0) ? FIRM_TARGET_AGB : FIRM_TARGET_TWL;
    FilterChoice filter = FILTER_CHOICE_SHARP_BILINEAR;
    if (strcmp(argv[2], "point") == 0) filter = FILTER_CHOICE_POINT;
    if (strcmp(argv[2], "bicubic") == 0) filter = FILTER_CHOICE_BICUBIC;

    const char* input_path = argv[3];
    const char* output_path = (argc >= 5) ? argv[4] : "patched.firm";

    if (firm_patch_file(input_path, output_path, target, filter)) {
        printf("Success: Patched %s -> %s\n", input_path, output_path);
        return 0;
    } else {
        printf("Failed to patch FIRM file.\n");
        return 1;
    }
}
