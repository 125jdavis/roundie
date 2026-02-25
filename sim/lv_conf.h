#ifndef LV_CONF_H
#define LV_CONF_H

/* Minimal LVGL config for PC simulator */

#define LV_USE_OS 0

#define LV_COLOR_DEPTH 16

#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

/* SDL display/input driver */
#define LV_USE_SDL 1

/* Fonts used by your screens */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_26 1
#define LV_FONT_MONTSERRAT_36 1

/* Optional but commonly needed */
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

#endif /* LV_CONF_H */
