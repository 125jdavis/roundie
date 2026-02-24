#pragma once

/* Minimal LVGL config for PC simulator */

#define LV_USE_OS 0

#define LV_COLOR_DEPTH 16

#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

/* Fonts used by your screens */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_28 1

/* Optional but commonly needed */
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0
