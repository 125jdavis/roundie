/**
 * screen_multiarc.h
 * Screen 1 – Multi-arc gauge display.
 *
 * Layout (466×466 round AMOLED, black background):
 *
 * Top section – two nested arcs, ~235° sweep:
 *   Outer arc  – Boost Pressure  (white/light-gray)
 *   Inner arc  – Lambda / AFR    (light-blue; red when boost>120 kPa AND lambda>1.1)
 *   Center     – Digital boost readout (large, bold, white)
 *
 * Bottom section – single arc, 135° sweep:
 *   Fuel Pressure arc (white/light-gray)
 *
 * Unit modes (controlled by global bool g_isMetric):
 *   Metric   – boost in kPa (0-300), lambda (0.7-1.3), fuel in kPa (0-500)
 *   Imperial – boost in psi (0-43.5), AFR (10.3-19.1), fuel in psi (0-75)
 */

#pragma once

#include <lvgl.h>
#include "config.h"
#include "can_handler.h"
#include "unit_convert.h"

extern bool g_isMetric;

// ── Widget handles ────────────────────────────────────────────────────────────
static lv_obj_t *s_maScreen        = nullptr;
static lv_obj_t *s_arcBoost        = nullptr;
static lv_obj_t *s_arcLambda       = nullptr;
static lv_obj_t *s_arcFuel         = nullptr;
static lv_obj_t *s_lblBoostVal     = nullptr;   // large center digital readout
static lv_obj_t *s_lblBoostUnit    = nullptr;   // "kPa" or "psi"

// ── Arc geometry ─────────────────────────────────────────────────────────────
// Top arcs: start=-145°, end=90°  → 235° sweep, opening at bottom
#define TOP_ARC_START_ANGLE     145   // LVGL: 0° = 3 o'clock, clockwise
#define TOP_ARC_END_ANGLE       35    // 145→360+35 = 250°… use LVGL convention:
                                       // start_angle=145, end_angle=35 wraps to 250°
// Using simpler convention: start=145 end=395(35+360)?
// LVGL lv_arc uses start_angle/end_angle in 0-360, clockwise from 3 o'clock.
// For a ~235° arc opening at the bottom:
//   Start at 7 o'clock (≈210°), end at 5 o'clock (≈150°) going clockwise.
//   That gives 360-210+150 = 300° ... not right.
// Actually we want start at ~152° and end ~28°, going anti-clockwise through top.
// Simplest: set arc bg mode, start=150, end=390 (i.e. 30 on the next wrap).
// We'll let LVGL handle it with LV_ARC_MODE_NORMAL and set the angles directly.

#define BOOST_ARC_SIZE          430   // outer arc diameter
#define LAMBDA_ARC_SIZE         390   // inner arc diameter
#define FUEL_ARC_SIZE           320   // bottom arc diameter

// ── Boost warning thresholds ─────────────────────────────────────────────────
#define BOOST_WARN_KPA          120.0f   // > 120 kPa absolute
#define LAMBDA_WARN             1.1f     // > 1.1 lambda

/**
 * Create all widgets for Screen 2.
 * @return pointer to the screen object
 */
static lv_obj_t *createMultiArcScreen(void) {
    s_maScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(s_maScreen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_maScreen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_maScreen, LV_OBJ_FLAG_SCROLLABLE);

    // ── Outer arc: Boost Pressure ─────────────────────────────────────────
    s_arcBoost = lv_arc_create(s_maScreen);
    lv_obj_set_size(s_arcBoost, BOOST_ARC_SIZE, BOOST_ARC_SIZE);
    lv_obj_align(s_arcBoost, LV_ALIGN_TOP_MID, 0, 5);
    // bg_angles: 150→30 clockwise = 240° sweep, centred at 12 o'clock (top)
    lv_arc_set_bg_angles(s_arcBoost, 150, 30);
    lv_arc_set_range(s_arcBoost, 0, 300);          // 0-300 kPa (metric default)
    lv_arc_set_value(s_arcBoost, 0);
    lv_arc_set_mode(s_arcBoost, LV_ARC_MODE_NORMAL);
    lv_obj_set_style_arc_color(s_arcBoost, lv_color_make(0xCC, 0xCC, 0xCC), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_arcBoost, 14, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_arcBoost, lv_color_make(0x30, 0x30, 0x30), LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_arcBoost, 14, LV_PART_MAIN);
    lv_obj_remove_style(s_arcBoost, nullptr, LV_PART_KNOB);
    lv_obj_clear_flag(s_arcBoost, LV_OBJ_FLAG_CLICKABLE);

    // ── Inner arc: Lambda / AFR ───────────────────────────────────────────
    s_arcLambda = lv_arc_create(s_maScreen);
    lv_obj_set_size(s_arcLambda, LAMBDA_ARC_SIZE, LAMBDA_ARC_SIZE);
    lv_obj_align(s_arcLambda, LV_ALIGN_TOP_MID, 0, 5 + (BOOST_ARC_SIZE - LAMBDA_ARC_SIZE) / 2);
    lv_arc_set_bg_angles(s_arcLambda, 150, 30);
    // Range internally stored × 1000 to keep integer precision:  700-1300
    lv_arc_set_range(s_arcLambda, 700, 1300);      // 0.7-1.3 lambda × 1000
    lv_arc_set_value(s_arcLambda, 700);
    lv_arc_set_mode(s_arcLambda, LV_ARC_MODE_NORMAL);
    lv_obj_set_style_arc_color(s_arcLambda, lv_color_make(0x00, 0xBF, 0xFF), LV_PART_INDICATOR); // light-blue
    lv_obj_set_style_arc_width(s_arcLambda, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_arcLambda, lv_color_make(0x30, 0x30, 0x30), LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_arcLambda, 10, LV_PART_MAIN);
    lv_obj_remove_style(s_arcLambda, nullptr, LV_PART_KNOB);
    lv_obj_clear_flag(s_arcLambda, LV_OBJ_FLAG_CLICKABLE);

    // ── Center digital boost readout ─────────────────────────────────────
    s_lblBoostVal = lv_label_create(s_maScreen);
    lv_label_set_text(s_lblBoostVal, "---");
    lv_obj_set_style_text_color(s_lblBoostVal, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_lblBoostVal, &lv_font_montserrat_36, 0);
    lv_obj_align(s_lblBoostVal, LV_ALIGN_CENTER, 0, -14);

    s_lblBoostUnit = lv_label_create(s_maScreen);
    lv_label_set_text(s_lblBoostUnit, "kPa");
    lv_obj_set_style_text_color(s_lblBoostUnit, lv_color_make(0xAA, 0xAA, 0xAA), 0);
    lv_obj_set_style_text_font(s_lblBoostUnit, &lv_font_montserrat_16, 0);
    lv_obj_align(s_lblBoostUnit, LV_ALIGN_CENTER, 0, 30);

    // ── Bottom arc: Fuel Pressure (135° sweep) ────────────────────────────
    s_arcFuel = lv_arc_create(s_maScreen);
    lv_obj_set_size(s_arcFuel, FUEL_ARC_SIZE, FUEL_ARC_SIZE);
    lv_obj_align(s_arcFuel, LV_ALIGN_BOTTOM_MID, 0, -10);
    // bg_angles: 22→158 clockwise = 136° sweep, centred at 6 o'clock (bottom)
    lv_arc_set_bg_angles(s_arcFuel, 22, 158);
    lv_arc_set_range(s_arcFuel, 0, 500);           // 0-500 kPa (metric default)
    lv_arc_set_value(s_arcFuel, 0);
    lv_arc_set_mode(s_arcFuel, LV_ARC_MODE_NORMAL);
    lv_obj_set_style_arc_color(s_arcFuel, lv_color_make(0xCC, 0xCC, 0xCC), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_arcFuel, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_arcFuel, lv_color_make(0x30, 0x30, 0x30), LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_arcFuel, 12, LV_PART_MAIN);
    lv_obj_remove_style(s_arcFuel, nullptr, LV_PART_KNOB);
    lv_obj_clear_flag(s_arcFuel, LV_OBJ_FLAG_CLICKABLE);

    return s_maScreen;
}

/**
 * Refresh Screen 2 with the latest sensor data.
 * Call from the main loop when this screen is active.
 */
static void updateMultiArcScreen(void) {
    if (!s_maScreen) return;

    // ── Boost arc ─────────────────────────────────────────────────────────
    float boostDisplay  = g_isMetric ? g_boostKpa : kPaToPsi(g_boostKpa);
    int32_t boostRange  = g_isMetric ? 300 : 44;   // 0-300 kPa or 0~43.5 psi
    lv_arc_set_range(s_arcBoost, 0, boostRange);
    lv_arc_set_value(s_arcBoost, (int32_t)boostDisplay);

    // Center readout
    static char boostBuf[16];
    snprintf(boostBuf, sizeof(boostBuf), "%.1f", boostDisplay);
    lv_label_set_text(s_lblBoostVal, boostBuf);
    lv_label_set_text(s_lblBoostUnit, g_isMetric ? "kPa" : "psi");

    // ── Lambda / AFR arc ──────────────────────────────────────────────────
    if (g_isMetric) {
        // Display lambda × 1000 so we can use integer arc range 700-1300
        lv_arc_set_range(s_arcLambda, 700, 1300);
        lv_arc_set_value(s_arcLambda, (int32_t)(g_lambda * 1000.0f));
    } else {
        // AFR mode: range 103-191 (× 10 for integer precision)
        lv_arc_set_range(s_arcLambda, 103, 191);
        lv_arc_set_value(s_arcLambda, (int32_t)(lambdaToAFR(g_lambda) * 10.0f));
    }

    // Lambda warning: red when boost > 120 kPa AND lambda > 1.1
    bool warnLean = (g_boostKpa > BOOST_WARN_KPA) && (g_lambda > LAMBDA_WARN);
    lv_color_t lambdaColor = warnLean
        ? lv_color_make(0xFF, 0x00, 0x00)      // red
        : lv_color_make(0x00, 0xBF, 0xFF);     // light-blue
    lv_obj_set_style_arc_color(s_arcLambda, lambdaColor, LV_PART_INDICATOR);

    // ── Fuel pressure arc ─────────────────────────────────────────────────
    float fuelDisplay = g_isMetric ? g_fuelPressKpa : kPaToPsi(g_fuelPressKpa);
    int32_t fuelRange = g_isMetric ? 500 : 75;
    lv_arc_set_range(s_arcFuel, 0, fuelRange);
    lv_arc_set_value(s_arcFuel, (int32_t)fuelDisplay);
}
