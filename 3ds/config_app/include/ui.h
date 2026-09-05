#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <stdbool.h>
#include "../../plugin_3gx/include/sharpscale_3ds.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int current_selection;
    Sharpscale3DSConfig config;
    bool needs_save;
} UiState;

void ui_init(void);
void ui_exit(void);
void ui_render_top(const UiState* state);
void ui_render_bottom(const UiState* state);
void ui_handle_input(UiState* state, uint32_t kDown);

#ifdef __cplusplus
}
#endif

#endif /* UI_H */
