#ifndef SHARPSCALE_CONFIG_H
#define SHARPSCALE_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "sharpscale_nx.h"

#ifdef __cplusplus
extern "C" {
#endif

void config_load_defaults(SharpscaleConfig* config);
bool config_load_global(SharpscaleConfig* config);
bool config_load_title(uint64_t title_id, SharpscaleConfig* config);
bool config_save_global(const SharpscaleConfig* config);
bool config_save_title(uint64_t title_id, const SharpscaleConfig* config);

const char* config_scaling_mode_to_string(SharpscaleScalingMode mode);
SharpscaleScalingMode config_string_to_scaling_mode(const char* str);

const char* config_filter_type_to_string(SharpscaleFilterType filter);
SharpscaleFilterType config_string_to_filter_type(const char* str);

const char* config_aspect_ratio_to_string(SharpscaleAspectRatio aspect);
SharpscaleAspectRatio config_string_to_aspect_ratio(const char* str);

#ifdef __cplusplus
}
#endif

#endif /* SHARPSCALE_CONFIG_H */
