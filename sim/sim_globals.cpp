#include <lvgl.h>
#include "config.h"
#include "Preferences.h"

// ── Unit system ───────────────────────────────────────────────────────────────
bool g_isMetric = true;

// ── Navigation state ─────────────────────────────────────────────────────────
int g_currentScreen = SCREEN_CLOCK;
int g_prevScreen    = SCREEN_CLOCK;

lv_obj_t* g_screens[4] = {};

// ── Sensor data (declared extern in can_handler.h) ────────────────────────────
float    g_lambda       = 1.0f;
float    g_boostKpa     = 0.0f;
float    g_fuelPressKpa = 0.0f;
uint16_t g_rpm          = 0;
float    g_coolantC     = 20.0f;
float    g_oilPressKpa  = 0.0f;

// ── NVS preferences stub ──────────────────────────────────────────────────────
Preferences g_prefs;