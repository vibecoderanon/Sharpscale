#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __3DS__
#include <3ds.h>
#endif

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UiState state;
    memset(&state, 0, sizeof(UiState));
    state.config.top_screen_mode = MODE_3DS_STANDARD_400PX;
    state.config.filter_mode = FILTER_3DS_CRISP_BILINEAR;
    state.config.enable_custom_bezel = true;

#ifdef __3DS__
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    while (aptMainLoop()) {
        hidScanInput();
        uint32_t kDown = hidKeysDown();

        if (kDown & KEY_START) break;

        ui_handle_input(&state, kDown);
        ui_render_top(&state);
        ui_render_bottom(&state);

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    gfxExit();
#else
    ui_render_top(&state);
#endif

    return 0;
}
