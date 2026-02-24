#include <lvgl.h>
#include <SDL.h>

#include "../roundie/config.h"
#include "../roundie/screen_clock.h"
#include "../roundie/screen_multiarc.h"
#include "../roundie/screen_boostgauge.h"
#include "../roundie/screen_setup.h"

extern lv_obj_t* g_screens[4];

static void switchToScreen(int idx) {
    if (idx < 0 || idx > SCREEN_SETUP) return;
    if (!g_screens[idx]) return;
    lv_scr_load(g_screens[idx]);
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) return 1;

    lv_init();

    // Provided by LVGL SDL driver sources we compile in CMake
    extern lv_display_t* lv_sdl_window_create(int32_t width, int32_t height);
    extern void lv_sdl_window_set_title(lv_display_t* disp, const char* title);
    extern void lv_sdl_window_update(void);

    lv_display_t* disp = lv_sdl_window_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_sdl_window_set_title(disp, "roundie LVGL9 simulator");

    g_screens[SCREEN_CLOCK]      = createClockScreen();
    g_screens[SCREEN_MULTIARC]   = createMultiArcScreen();
    g_screens[SCREEN_BOOSTGAUGE] = createAnalogBoostScreen();
    g_screens[SCREEN_SETUP]      = createSetupScreen();

    switchToScreen(SCREEN_CLOCK);

    bool running = true;
    uint32_t last = SDL_GetTicks();

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE: running = false; break;
                    case SDLK_1: switchToScreen(SCREEN_CLOCK); break;
                    case SDLK_2: switchToScreen(SCREEN_MULTIARC); break;
                    case SDLK_3: switchToScreen(SCREEN_BOOSTGAUGE); break;
                    case SDLK_4: switchToScreen(SCREEN_SETUP); break;
                    default: break;
                }
            }
        }

        uint32_t now = SDL_GetTicks();
        lv_tick_inc(now - last);
        last = now;

        lv_timer_handler();
        lv_sdl_window_update();
        SDL_Delay(5);
    }

    SDL_Quit();
    return 0;
}