#include <lvgl.h>
#include "../roundie/config.h"

bool g_isMetric = true;

int g_currentScreen = SCREEN_CLOCK;
int g_prevScreen    = SCREEN_CLOCK;

lv_obj_t* g_screens[4] = {};