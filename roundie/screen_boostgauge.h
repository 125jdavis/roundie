/**
 * screen_boostgauge.h
 * Screen 2 – Analog boost gauge (traditional needle style).
 *
 * Layout (466×466 round AMOLED):
 *   - Black background
 *   - White dial with major ticks at 0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0 bar
 *   - Minor tick subdivisions (5 per major interval → 0.1 bar each)
 *   - Numeric labels for major ticks
 *   - Orange needle, smoothly animated
 *   - "bar" or "psi" unit label below center
 *
 * Range: 0–3.0 bar absolute (0–300 kPa, 0–43.5 psi)
 *
 * Implemented with lv_meter (LVGL 8) – lv_scale is used in LVGL 9.
 * Compile-time guard selects the correct API.
 */

#pragma once

#include <lvgl.h>
#include "config.h"
#include "can_handler.h"
#include "unit_convert.h"

extern bool g_isMetric;

static lv_obj_t *s_bgScreen      = nullptr;  // Screen 3 container
static lv_obj_t *s_bgMeter       = nullptr;  // lv_meter widget
static lv_obj_t *s_bgUnitLabel   = nullptr;  // "bar" / "psi"

#if LVGL_VERSION_MAJOR >= 9
// ── LVGL 9 implementation (lv_scale) ─────────────────────────────────────────
static lv_obj_t *s_bgScale = nullptr;

static lv_obj_t *createAnalogBoostScreen(void) {
    s_bgScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(s_bgScreen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_bgScreen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_bgScreen, LV_OBJ_FLAG_SCROLLABLE);

    // Use lv_scale in round/arc mode
    s_bgScale = lv_scale_create(s_bgScreen);
    lv_obj_set_size(s_bgScale, 400, 400);
    lv_obj_align(s_bgScale, LV_ALIGN_CENTER, 0, 0);
    lv_scale_set_mode(s_bgScale, LV_SCALE_MODE_ROUND_INNER);

    // Range: 0-300 (kPa × 0.1 = bar; we display bar so map 0-30 internally → ×10)
    lv_scale_set_range(s_bgScale, 0, 300);
    lv_scale_set_total_tick_count(s_bgScale, 31);   // 0 to 30 → 31 ticks (×10 = 0–300 kPa)
    lv_scale_set_major_tick_every(s_bgScale, 5);    // every 0.5 bar
    lv_obj_set_style_length(s_bgScale, 20, LV_PART_INDICATOR);
    lv_obj_set_style_length(s_bgScale, 10, LV_PART_ITEMS);

    // Needle (orange)
    static int32_t needleVal = 0;
    lv_scale_set_line_needle_value(s_bgScale, s_bgScale, 150, needleVal);

    // Color scheme
    lv_obj_set_style_arc_color(s_bgScale, lv_color_make(0x44, 0x44, 0x44), LV_PART_MAIN);
    lv_obj_set_style_line_color(s_bgScale, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_line_color(s_bgScale, lv_color_make(0x80, 0x80, 0x80), LV_PART_ITEMS);
    lv_obj_set_style_text_color(s_bgScale, lv_color_white(), LV_PART_INDICATOR);

    // Unit label
    s_bgUnitLabel = lv_label_create(s_bgScreen);
    lv_label_set_text(s_bgUnitLabel, "bar");
    lv_obj_set_style_text_color(s_bgUnitLabel, lv_color_make(0xAA, 0xAA, 0xAA), 0);
    lv_obj_set_style_text_font(s_bgUnitLabel, &lv_font_montserrat_20, 0);
    lv_obj_align(s_bgUnitLabel, LV_ALIGN_CENTER, 0, 80);

    return s_bgScreen;
}

static void updateAnalogBoostScreen(void) {
    if (!s_bgScreen || !s_bgScale) return;
    // Map kPa (0-300) → internal scale (0-300, 1:1)
    float kpa = g_boostKpa < 0.0f ? 0.0f : (g_boostKpa > 300.0f ? 300.0f : g_boostKpa);
    static int32_t needleVal;
    needleVal = (int32_t)kpa;
    lv_scale_set_line_needle_value(s_bgScale, s_bgScale, 150, needleVal);

    if (!g_isMetric) {
        lv_label_set_text(s_bgUnitLabel, "psi");
    } else {
        lv_label_set_text(s_bgUnitLabel, "bar");
    }
}

#else
// ── LVGL 8 implementation (lv_meter) ─────────────────────────────────────────
static lv_meter_indicator_t *s_bgNeedle = nullptr;

static lv_obj_t *createAnalogBoostScreen(void) {
    s_bgScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(s_bgScreen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_bgScreen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_bgScreen, LV_OBJ_FLAG_SCROLLABLE);

    s_bgMeter = lv_meter_create(s_bgScreen);
    lv_obj_set_size(s_bgMeter, 420, 420);
    lv_obj_align(s_bgMeter, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(s_bgMeter, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_bgMeter, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_bgMeter, 0, 0);

    // Add a scale: 0–300 kPa (represents 0–3.0 bar at ×0.1 scale)
    lv_meter_scale_t *scale = lv_meter_add_scale(s_bgMeter);
    // 235° sweep, start at lower-left, end at lower-right
    lv_meter_set_scale_range(s_bgMeter, scale, 0, 300, 235, 152);
    lv_meter_set_scale_ticks(s_bgMeter, scale, 61, 2, 10, lv_color_make(0x80, 0x80, 0x80));
    // Major ticks every 50 kPa (= 0.5 bar)
    lv_meter_set_scale_major_ticks(s_bgMeter, scale, 10, 4, 20, lv_color_white(), 14);

    // Orange needle
    lv_color_t orange = lv_color_make(0xFF, 0x80, 0x00);
    s_bgNeedle = lv_meter_add_needle_line(s_bgMeter, scale, 4, orange, -40);

    // Set initial value
    lv_meter_set_indicator_value(s_bgMeter, s_bgNeedle, 0);

    // Unit label
    s_bgUnitLabel = lv_label_create(s_bgScreen);
    lv_label_set_text(s_bgUnitLabel, "bar");
    lv_obj_set_style_text_color(s_bgUnitLabel, lv_color_make(0xAA, 0xAA, 0xAA), 0);
    lv_obj_set_style_text_font(s_bgUnitLabel, &lv_font_montserrat_20, 0);
    lv_obj_align(s_bgUnitLabel, LV_ALIGN_CENTER, 0, 80);

    return s_bgScreen;
}

static void updateAnalogBoostScreen(void) {
    if (!s_bgScreen || !s_bgMeter) return;

    // Clamp to 0-300 kPa
    float kpa = g_boostKpa < 0.0f ? 0.0f : (g_boostKpa > 300.0f ? 300.0f : g_boostKpa);
    lv_meter_set_indicator_value(s_bgMeter, s_bgNeedle, (int32_t)kpa);

    lv_label_set_text(s_bgUnitLabel, g_isMetric ? "bar" : "psi");
}
#endif  // LVGL_VERSION_MAJOR >= 9
