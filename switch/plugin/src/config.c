#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void config_load_defaults(SharpscaleConfig* config) {
    if (!config) return;
    memset(config, 0, sizeof(SharpscaleConfig));

    config->title_id = 0;
    config->scaling_mode = SCALING_MODE_INTEGER;
    config->filter_type = FILTER_TYPE_POINT;
    config->aspect_ratio = ASPECT_RATIO_AUTO;
    config->sharpness_strength = 80;
    config->force_1080p_capture = true;
    config->show_osd_notification = true;
    config->border_color_rgba = 0x000000FF; // Solid black letterboxing

    config->src_width = 1280;
    config->src_height = 720;
    config->dst_width = 1280;
    config->dst_height = 720;
    config->is_docked = false;

    config->calculated_viewport.x = 0;
    config->calculated_viewport.y = 0;
    config->calculated_viewport.width = 1280;
    config->calculated_viewport.height = 720;
    config->scale_factor_x = 1.0f;
    config->scale_factor_y = 1.0f;
}

const char* config_scaling_mode_to_string(SharpscaleScalingMode mode) {
    switch (mode) {
        case SCALING_MODE_ORIGINAL: return "original";
        case SCALING_MODE_INTEGER:  return "integer";
        case SCALING_MODE_REAL:     return "real";
        case SCALING_MODE_FIT:      return "fit";
        case SCALING_MODE_CUSTOM:   return "custom";
        default:                    return "original";
    }
}

SharpscaleScalingMode config_string_to_scaling_mode(const char* str) {
    if (!str) return SCALING_MODE_ORIGINAL;
    if (strcmp(str, "integer") == 0) return SCALING_MODE_INTEGER;
    if (strcmp(str, "real") == 0)    return SCALING_MODE_REAL;
    if (strcmp(str, "fit") == 0)     return SCALING_MODE_FIT;
    if (strcmp(str, "custom") == 0)  return SCALING_MODE_CUSTOM;
    return SCALING_MODE_ORIGINAL;
}

const char* config_filter_type_to_string(SharpscaleFilterType filter) {
    switch (filter) {
        case FILTER_TYPE_POINT:          return "point";
        case FILTER_TYPE_BILINEAR:       return "bilinear";
        case FILTER_TYPE_SHARP_BILINEAR: return "sharp_bilinear";
        case FILTER_TYPE_CAS:            return "cas";
        case FILTER_TYPE_BICUBIC:        return "bicubic";
        default:                         return "point";
    }
}

SharpscaleFilterType config_string_to_filter_type(const char* str) {
    if (!str) return FILTER_TYPE_POINT;
    if (strcmp(str, "point") == 0)          return FILTER_TYPE_POINT;
    if (strcmp(str, "bilinear") == 0)       return FILTER_TYPE_BILINEAR;
    if (strcmp(str, "sharp_bilinear") == 0) return FILTER_TYPE_SHARP_BILINEAR;
    if (strcmp(str, "cas") == 0)            return FILTER_TYPE_CAS;
    if (strcmp(str, "bicubic") == 0)        return FILTER_TYPE_BICUBIC;
    return FILTER_TYPE_POINT;
}

const char* config_aspect_ratio_to_string(SharpscaleAspectRatio aspect) {
    switch (aspect) {
        case ASPECT_RATIO_AUTO: return "auto";
        case ASPECT_RATIO_16_9: return "16:9";
        case ASPECT_RATIO_4_3:  return "4:3";
        case ASPECT_RATIO_3_2:  return "3:2";
        case ASPECT_RATIO_1_1:  return "1:1";
        case ASPECT_RATIO_10_9: return "10:9";
        default:                return "auto";
    }
}

SharpscaleAspectRatio config_string_to_aspect_ratio(const char* str) {
    if (!str) return ASPECT_RATIO_AUTO;
    if (strcmp(str, "16:9") == 0) return ASPECT_RATIO_16_9;
    if (strcmp(str, "4:3") == 0)  return ASPECT_RATIO_4_3;
    if (strcmp(str, "3:2") == 0)  return ASPECT_RATIO_3_2;
    if (strcmp(str, "1:1") == 0)  return ASPECT_RATIO_1_1;
    if (strcmp(str, "10:9") == 0) return ASPECT_RATIO_10_9;
    return ASPECT_RATIO_AUTO;
}

static bool parse_ini_file(const char* filepath, SharpscaleConfig* config) {
    FILE* f = fopen(filepath, "r");
    if (!f) return false;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char key[64], val[64];
        if (sscanf(line, " %63[^= ] = %63s", key, val) == 2) {
            if (strcmp(key, "scaling_mode") == 0) {
                config->scaling_mode = config_string_to_scaling_mode(val);
            } else if (strcmp(key, "filter_type") == 0) {
                config->filter_type = config_string_to_filter_type(val);
            } else if (strcmp(key, "aspect_ratio") == 0) {
                config->aspect_ratio = config_string_to_aspect_ratio(val);
            } else if (strcmp(key, "sharpness") == 0) {
                config->sharpness_strength = (uint8_t)atoi(val);
            } else if (strcmp(key, "force_1080p_capture") == 0) {
                config->force_1080p_capture = (atoi(val) != 0);
            } else if (strcmp(key, "show_osd") == 0) {
                config->show_osd_notification = (atoi(val) != 0);
            }
        }
    }

    fclose(f);
    return true;
}

bool config_load_global(SharpscaleConfig* config) {
    return parse_ini_file(SHARPSCALE_GLOBAL_CONFIG, config);
}

bool config_load_title(uint64_t title_id, SharpscaleConfig* config) {
    char path[128];
    snprintf(path, sizeof(path), "%s/%016llX.ini", SHARPSCALE_TITLE_CONFIG_DIR, (unsigned long long)title_id);
    return parse_ini_file(path, config);
}

static bool write_ini_file(const char* filepath, const SharpscaleConfig* config) {
    FILE* f = fopen(filepath, "w");
    if (!f) return false;

    fprintf(f, "# Sharpscale-NX Configuration\n");
    fprintf(f, "scaling_mode = %s\n", config_scaling_mode_to_string(config->scaling_mode));
    fprintf(f, "filter_type = %s\n", config_filter_type_to_string(config->filter_type));
    fprintf(f, "aspect_ratio = %s\n", config_aspect_ratio_to_string(config->aspect_ratio));
    fprintf(f, "sharpness = %u\n", config->sharpness_strength);
    fprintf(f, "force_1080p_capture = %d\n", config->force_1080p_capture ? 1 : 0);
    fprintf(f, "show_osd = %d\n", config->show_osd_notification ? 1 : 0);

    fclose(f);
    return true;
}

bool config_save_global(const SharpscaleConfig* config) {
    return write_ini_file(SHARPSCALE_GLOBAL_CONFIG, config);
}

bool config_save_title(uint64_t title_id, const SharpscaleConfig* config) {
    char path[128];
    snprintf(path, sizeof(path), "%s/%016llX.ini", SHARPSCALE_TITLE_CONFIG_DIR, (unsigned long long)title_id);
    return write_ini_file(path, config);
}
