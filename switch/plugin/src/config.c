#include "config.h"

#ifdef __SWITCH_PLUGIN__

/* In plugin context (running inside retail game process), standard libc file/string I/O
 * from newlib is non-functional and triggers instruction aborts.
 * We provide zero-dependency inline string helpers and optional SaltySD file I/O. */

static inline int fast_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

static inline bool fast_memcmp_eq(const char* s, const char* target, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (s[i] != target[i]) return false;
    }
    return target[len] == '\0';
}

static inline uint32_t fast_parse_u32(const char* s, size_t len) {
    uint32_t val = 0;
    for (size_t i = 0; i < len; i++) {
        if (s[i] >= '0' && s[i] <= '9') {
            val = val * 10 + (s[i] - '0');
        } else {
            break;
        }
    }
    return val;
}

/* Weak SaltySD symbols for optional SD card file I/O */
extern uint64_t SaltySDCore_fopen(const char* filename, const char* mode) __attribute__((weak));
extern uint64_t SaltySDCore_fread(void* ptr, size_t size, size_t count, uint64_t stream) __attribute__((weak));
extern uint64_t SaltySDCore_fclose(uint64_t stream) __attribute__((weak));

#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define fast_strcmp strcmp

static void ensure_dir_exists(const char* dirpath) {
    struct stat st;
    if (stat(dirpath, &st) != 0) {
        mkdir(dirpath, 0777);
    }
}

#endif

void config_load_defaults(SharpscaleConfig* config) {
    if (!config) return;
    uint8_t* p = (uint8_t*)config;
    for (size_t i = 0; i < sizeof(SharpscaleConfig); i++) p[i] = 0;

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
    if (fast_strcmp(str, "integer") == 0) return SCALING_MODE_INTEGER;
    if (fast_strcmp(str, "real") == 0)    return SCALING_MODE_REAL;
    if (fast_strcmp(str, "fit") == 0)     return SCALING_MODE_FIT;
    if (fast_strcmp(str, "custom") == 0)  return SCALING_MODE_CUSTOM;
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
    if (fast_strcmp(str, "point") == 0)          return FILTER_TYPE_POINT;
    if (fast_strcmp(str, "bilinear") == 0)       return FILTER_TYPE_BILINEAR;
    if (fast_strcmp(str, "sharp_bilinear") == 0) return FILTER_TYPE_SHARP_BILINEAR;
    if (fast_strcmp(str, "cas") == 0)            return FILTER_TYPE_CAS;
    if (fast_strcmp(str, "bicubic") == 0)        return FILTER_TYPE_BICUBIC;
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
    if (fast_strcmp(str, "16:9") == 0) return ASPECT_RATIO_16_9;
    if (fast_strcmp(str, "4:3") == 0)  return ASPECT_RATIO_4_3;
    if (fast_strcmp(str, "3:2") == 0)  return ASPECT_RATIO_3_2;
    if (fast_strcmp(str, "1:1") == 0)  return ASPECT_RATIO_1_1;
    if (fast_strcmp(str, "10:9") == 0) return ASPECT_RATIO_10_9;
    return ASPECT_RATIO_AUTO;
}

#ifdef __SWITCH_PLUGIN__

static void parse_ini_buffer(const char* buf, size_t buf_len, SharpscaleConfig* config) {
    size_t i = 0;
    while (i < buf_len && buf[i] != '\0') {
        // Skip leading whitespace / newlines
        while (i < buf_len && (buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\r' || buf[i] == '\n')) i++;
        if (i >= buf_len || buf[i] == '\0') break;

        // Skip comments and section headers
        if (buf[i] == '#' || buf[i] == ';' || buf[i] == '[') {
            while (i < buf_len && buf[i] != '\n' && buf[i] != '\0') i++;
            continue;
        }

        // Key
        size_t key_start = i;
        while (i < buf_len && buf[i] != '=' && buf[i] != ' ' && buf[i] != '\t' && buf[i] != '\r' && buf[i] != '\n' && buf[i] != '\0') i++;
        size_t key_len = i - key_start;

        while (i < buf_len && (buf[i] == ' ' || buf[i] == '\t')) i++;
        if (i < buf_len && buf[i] == '=') i++;
        while (i < buf_len && (buf[i] == ' ' || buf[i] == '\t')) i++;

        // Value
        size_t val_start = i;
        while (i < buf_len && buf[i] != ' ' && buf[i] != '\t' && buf[i] != '\r' && buf[i] != '\n' && buf[i] != '\0') i++;
        size_t val_len = i - val_start;

        if (key_len > 0 && val_len > 0) {
            const char* k = buf + key_start;
            const char* v = buf + val_start;

            if (fast_memcmp_eq(k, "scaling_mode", key_len)) {
                if (fast_memcmp_eq(v, "integer", val_len)) config->scaling_mode = SCALING_MODE_INTEGER;
                else if (fast_memcmp_eq(v, "real", val_len)) config->scaling_mode = SCALING_MODE_REAL;
                else if (fast_memcmp_eq(v, "fit", val_len)) config->scaling_mode = SCALING_MODE_FIT;
                else if (fast_memcmp_eq(v, "custom", val_len)) config->scaling_mode = SCALING_MODE_CUSTOM;
                else if (fast_memcmp_eq(v, "original", val_len)) config->scaling_mode = SCALING_MODE_ORIGINAL;
                else config->scaling_mode = (SharpscaleScalingMode)fast_parse_u32(v, val_len);
            } else if (fast_memcmp_eq(k, "filter_type", key_len)) {
                if (fast_memcmp_eq(v, "point", val_len)) config->filter_type = FILTER_TYPE_POINT;
                else if (fast_memcmp_eq(v, "bilinear", val_len)) config->filter_type = FILTER_TYPE_BILINEAR;
                else if (fast_memcmp_eq(v, "sharp_bilinear", val_len)) config->filter_type = FILTER_TYPE_SHARP_BILINEAR;
                else if (fast_memcmp_eq(v, "cas", val_len)) config->filter_type = FILTER_TYPE_CAS;
                else if (fast_memcmp_eq(v, "bicubic", val_len)) config->filter_type = FILTER_TYPE_BICUBIC;
                else config->filter_type = (SharpscaleFilterType)fast_parse_u32(v, val_len);
            } else if (fast_memcmp_eq(k, "aspect_ratio", key_len)) {
                if (fast_memcmp_eq(v, "auto", val_len)) config->aspect_ratio = ASPECT_RATIO_AUTO;
                else if (fast_memcmp_eq(v, "16:9", val_len)) config->aspect_ratio = ASPECT_RATIO_16_9;
                else if (fast_memcmp_eq(v, "4:3", val_len)) config->aspect_ratio = ASPECT_RATIO_4_3;
                else if (fast_memcmp_eq(v, "3:2", val_len)) config->aspect_ratio = ASPECT_RATIO_3_2;
                else if (fast_memcmp_eq(v, "1:1", val_len)) config->aspect_ratio = ASPECT_RATIO_1_1;
                else if (fast_memcmp_eq(v, "10:9", val_len)) config->aspect_ratio = ASPECT_RATIO_10_9;
                else config->aspect_ratio = (SharpscaleAspectRatio)fast_parse_u32(v, val_len);
            } else if (fast_memcmp_eq(k, "sharpness", key_len)) {
                config->sharpness_strength = (uint8_t)fast_parse_u32(v, val_len);
            } else if (fast_memcmp_eq(k, "force_1080p_capture", key_len) || fast_memcmp_eq(k, "force_1080p", key_len)) {
                config->force_1080p_capture = (fast_parse_u32(v, val_len) != 0);
            } else if (fast_memcmp_eq(k, "show_osd", key_len)) {
                config->show_osd_notification = (fast_parse_u32(v, val_len) != 0);
            }
        }

        while (i < buf_len && buf[i] != '\n' && buf[i] != '\0') i++;
    }
}

static bool load_ini_via_saltysd(const char* path, SharpscaleConfig* config) {
    if (&SaltySDCore_fopen && &SaltySDCore_fread && &SaltySDCore_fclose) {
        uint64_t handle = SaltySDCore_fopen(path, "r");
        if (handle) {
            char buf[1024];
            size_t read_bytes = SaltySDCore_fread(buf, 1, sizeof(buf) - 1, handle);
            buf[read_bytes] = '\0';
            SaltySDCore_fclose(handle);
            if (read_bytes > 0) {
                parse_ini_buffer(buf, read_bytes, config);
                return true;
            }
        }
    }
    return false;
}

bool config_load_global(SharpscaleConfig* config) {
    return load_ini_via_saltysd("sdmc:" SHARPSCALE_GLOBAL_CONFIG, config);
}

static void u64_to_hex(uint64_t val, char* out) {
    const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 15; i >= 0; i--) {
        out[i] = hex_chars[val & 0xF];
        val >>= 4;
    }
    out[16] = '\0';
}

bool config_load_title(uint64_t title_id, SharpscaleConfig* config) {
    char hex_id[17];
    u64_to_hex(title_id, hex_id);
    char path[128] = "sdmc:" SHARPSCALE_TITLE_CONFIG_DIR "/";
    size_t base_len = 0;
    while (path[base_len]) base_len++;
    for (int i = 0; i < 16; i++) path[base_len + i] = hex_id[i];
    path[base_len + 16] = '.';
    path[base_len + 17] = 'i';
    path[base_len + 18] = 'n';
    path[base_len + 19] = 'i';
    path[base_len + 20] = '\0';
    return load_ini_via_saltysd(path, config);
}

bool config_save_global(const SharpscaleConfig* config) {
    (void)config;
    return false;
}

bool config_save_title(uint64_t title_id, const SharpscaleConfig* config) {
    (void)title_id;
    (void)config;
    return false;
}

#else

/* Overlay Implementation using standard libc file I/O */

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
            } else if (strcmp(key, "force_1080p_capture") == 0 || strcmp(key, "force_1080p") == 0) {
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
    ensure_dir_exists("/switch");
    ensure_dir_exists(SHARPSCALE_CONFIG_DIR);
    ensure_dir_exists(SHARPSCALE_TITLE_CONFIG_DIR);

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

#endif

