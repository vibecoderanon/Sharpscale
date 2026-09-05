#include "ui.h"
#include <stdio.h>
#include <string.h>

static const char* MENU_ITEMS[] = {
    "Top Screen Mode",
    "Scaling Filter",
    "Retro Geometry Profile",
    "Custom Bezels & Borders",
    "Generate Patched TWL_FIRM",
    "Generate Patched AGB_FIRM",
    "Save and Apply Settings"
};

#define MENU_ITEM_COUNT (sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]))

void ui_init(void) {
    // Console setup
}

void ui_exit(void) {
    // Console teardown
}

void ui_render_top(const UiState* state) {
    printf("\x1b[1;1H"); // ANSI cursor home
    printf("========================================\n");
    printf("        Sharpscale-3DS Configurator     \n");
    printf("                 v%s            \n", SHARPSCALE_3DS_VERSION_STRING);
    printf("========================================\n\n");

    printf(" Mode:   %s\n", (state->config.top_screen_mode == MODE_3DS_HIRES_800PX) ? "800x240 High-Density 2D" : "400x240 Standard 3D");
    printf(" Filter: %s\n", (state->config.filter_mode == FILTER_3DS_POINT) ? "Point (Nearest Neighbor)" :
                           (state->config.filter_mode == FILTER_3DS_CRISP_BILINEAR) ? "Sharp Bilinear (Crisp)" : "Smooth Bilinear");
    printf(" Bezel:  %s\n\n", state->config.enable_custom_bezel ? "Enabled (Solid / Styled)" : "Disabled (Black Letterbox)");

    printf("----------------------------------------\n");
    for (size_t i = 0; i < MENU_ITEM_COUNT; i++) {
        if ((int)i == state->current_selection) {
            printf(" > [%c] %s\n", '*', MENU_ITEMS[i]);
        } else {
            printf("    [ ] %s\n", MENU_ITEMS[i]);
        }
    }
    printf("----------------------------------------\n");
    printf(" Controls: D-Pad Up/Down: Navigate\n");
    printf("           A: Toggle/Execute | START: Exit\n");
}

void ui_render_bottom(const UiState* state) {
    (void)state;
    // Bottom touch screen hints or graphical preview
}

void ui_handle_input(UiState* state, uint32_t kDown) {
    // 3DS KEY_DUP = 0x40, KEY_DDOWN = 0x80, KEY_A = 0x01, KEY_B = 0x02
    if (kDown & 0x40) { // Up
        if (state->current_selection > 0) state->current_selection--;
    }
    if (kDown & 0x80) { // Down
        if (state->current_selection < (int)MENU_ITEM_COUNT - 1) state->current_selection++;
    }
    if (kDown & 0x01) { // A
        switch (state->current_selection) {
            case 0:
                state->config.top_screen_mode = (state->config.top_screen_mode == MODE_3DS_HIRES_800PX) ?
                    MODE_3DS_STANDARD_400PX : MODE_3DS_HIRES_800PX;
                state->needs_save = true;
                break;
            case 1:
                state->config.filter_mode = (Sharpscale3DSFilter)((state->config.filter_mode + 1) % 3);
                state->needs_save = true;
                break;
            case 2:
                state->config.retro_format = (SharpscaleRetroFormat)((state->config.retro_format + 1) % 7);
                state->needs_save = true;
                break;
            case 3:
                state->config.enable_custom_bezel = !state->config.enable_custom_bezel;
                state->needs_save = true;
                break;
            default:
                break;
        }
    }
}
