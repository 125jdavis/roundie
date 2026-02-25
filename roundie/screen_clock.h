/**
 * screen_clock.h
 * Screen 0 – Analog clock sourced from the PCF85063 RTC.
 *
 * Layout (466×466 round AMOLED):
 *   - Black background
 *   - White dial with 12, 3, 6, 9 numerals and tick marks
 *   - Orange hour, minute, and (thin) second hands
 *   - Centered on the round display
 *
 * Requires LVGL 8.x or 9.x.
 */

#pragma once

#include <lvgl.h>
#include "config.h"

// ── Clock screen objects (file-scoped) ───────────────────────────────────────
static lv_obj_t  *s_clockScreen    = nullptr;
static lv_obj_t  *s_clockCanvas    = nullptr;  // canvas for custom clock drawing
static lv_obj_t  *s_hourHand       = nullptr;
static lv_obj_t  *s_minuteHand     = nullptr;
static lv_obj_t  *s_secondHand     = nullptr;
static lv_obj_t  *s_clockCenter    = nullptr;  // center dot

// Clock face numerals
static lv_obj_t  *s_numLabels[4]   = {};       // 12, 3, 6, 9

// ── Constants ────────────────────────────────────────────────────────────────
#define CLOCK_CX        (DISPLAY_WIDTH  / 2)   // 233
#define CLOCK_CY        (DISPLAY_HEIGHT / 2)   // 233
#define CLOCK_R         210                    // outer radius of dial ring
#define HOUR_LEN        100
#define MIN_LEN         140
#define SEC_LEN         160

// Degrees to radians
static inline float _deg2rad(float deg) { return deg * (float)M_PI / 180.0f; }

/**
 * Helper – create a thin line object to represent a clock hand.
 * The hand rotates around the screen center; LVGL transform pivot is set to
 * the bottom mid-point of the image so it pivots at the clock center.
 *
 * We draw each hand as a 4-px-wide, length-px-tall rectangle with a pivot at
 * the bottom center, then rotate via lv_obj_set_style_transform_rotation().
 */
static lv_obj_t *_createHand(lv_obj_t *parent, int32_t length, int32_t width,
                              lv_color_t color) {
    lv_obj_t *hand = lv_obj_create(parent);
    lv_obj_remove_style_all(hand);
    lv_obj_set_size(hand, width, length);
    lv_obj_set_style_bg_color(hand, color, 0);
    lv_obj_set_style_bg_opa(hand, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(hand, width / 2, 0);
    // Pivot at bottom-center of the hand rectangle so it rotates around CX,CY
    lv_obj_set_style_transform_pivot_x(hand, width / 2, 0);
    lv_obj_set_style_transform_pivot_y(hand, length, 0);
    // Place the pivot point at the screen center
    lv_obj_set_pos(hand, CLOCK_CX - width / 2, CLOCK_CY - length);
    return hand;
}

/**
 * Draw major and minor tick marks directly on the clock screen background.
 * Called once during createClockScreen().
 */
static void _drawTicks(lv_obj_t *parent) {
    for (int i = 0; i < 60; i++) {
        float angle = _deg2rad(i * 6.0f - 90.0f);
        bool isMajor = (i % 5 == 0);
        int32_t outerR = CLOCK_R;
        int32_t innerR = isMajor ? CLOCK_R - 20 : CLOCK_R - 10;

        lv_obj_t *tick = lv_obj_create(parent);
        lv_obj_remove_style_all(tick);

        int32_t x1 = (int32_t)(CLOCK_CX + cosf(angle) * innerR);
        int32_t y1 = (int32_t)(CLOCK_CY + sinf(angle) * innerR);
        int32_t x2 = (int32_t)(CLOCK_CX + cosf(angle) * outerR);
        int32_t y2 = (int32_t)(CLOCK_CY + sinf(angle) * outerR);

        int32_t dx = x2 - x1;
        int32_t dy = y2 - y1;
        int32_t len = (int32_t)sqrtf((float)(dx * dx + dy * dy));
        int32_t w   = isMajor ? 3 : 2;

        lv_obj_set_size(tick, w, len);
        lv_obj_set_style_bg_color(tick, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(tick, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(tick, 1, 0);

        // Position tick center at midpoint between inner and outer radius
        int32_t midX = (x1 + x2) / 2;
        int32_t midY = (y1 + y2) / 2;
        lv_obj_set_pos(tick, midX - w / 2, midY - len / 2);

        // Rotate the tick to point radially
        float angleDeg = (float)i * 6.0f;  // 0° at 12 o'clock, then clockwise
        lv_obj_set_style_transform_rotation(tick, (int32_t)(angleDeg * 10.0f), 0);
        lv_obj_set_style_transform_pivot_x(tick, w / 2, 0);
        lv_obj_set_style_transform_pivot_y(tick, len / 2, 0);
    }
}

/**
 * Create all LVGL widgets for the clock screen.
 * @return  pointer to the created screen object
 */
static lv_obj_t *createClockScreen(void) {
    s_clockScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(s_clockScreen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_clockScreen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_clockScreen, LV_OBJ_FLAG_SCROLLABLE);

    // Draw tick marks
    _drawTicks(s_clockScreen);

    // Hour numerals: 12, 3, 6, 9
    static const char *numerals[] = { "12", "3", "6", "9" };
    static const float numAngles[] = { -90.0f, 0.0f, 90.0f, 180.0f };
    static const int32_t numR = CLOCK_R - 40;

    for (int i = 0; i < 4; i++) {
        lv_obj_t *lbl = lv_label_create(s_clockScreen);
        lv_label_set_text(lbl, numerals[i]);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);

        float rad = _deg2rad(numAngles[i]);
        int32_t x = CLOCK_CX + (int32_t)(cosf(rad) * numR);
        int32_t y = CLOCK_CY + (int32_t)(sinf(rad) * numR);
        lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_set_pos(lbl, x - 10, y - 12);
        s_numLabels[i] = lbl;
    }

    // Clock hands (orange)
    lv_color_t orange = lv_color_make(0xFF, 0x80, 0x00);
    s_hourHand   = _createHand(s_clockScreen, HOUR_LEN, 6,  orange);
    s_minuteHand = _createHand(s_clockScreen, MIN_LEN,  4,  orange);
    s_secondHand = _createHand(s_clockScreen, SEC_LEN,  2,  orange);

    // Center dot (orange cap)
    s_clockCenter = lv_obj_create(s_clockScreen);
    lv_obj_remove_style_all(s_clockCenter);
    lv_obj_set_size(s_clockCenter, 12, 12);
    lv_obj_set_style_bg_color(s_clockCenter, orange, 0);
    lv_obj_set_style_bg_opa(s_clockCenter, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_clockCenter, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(s_clockCenter, LV_ALIGN_CENTER, 0, 0);

    return s_clockScreen;
}

/**
 * Update the clock hand angles from the provided hour/minute/second values.
 * Call this once per second from the main loop.
 *
 * @param hour    0-23
 * @param minute  0-59
 * @param second  0-59
 */
static void updateClockScreen(uint8_t hour, uint8_t minute, uint8_t second) {
    if (!s_clockScreen) return;

    // Convert to 12-hour for the clock face
    uint8_t h12 = hour % 12;

    // Angles in tenths-of-degrees (LVGL transform_rotation unit)
    // 12 o'clock = -90° (or 270°).  We use 0° = 12 o'clock convention:
    //   second hand: 6°/sec,  minute hand: 0.1°/sec,  hour hand: 0.00833°/sec
    int32_t secAngle  = (int32_t)(second * 6)            * 10;  // 6° per second
    int32_t minAngle  = (int32_t)(minute * 6 + second / 10) * 10;
    int32_t hourAngle = (int32_t)((h12 * 30) + (minute / 2)) * 10;

    lv_obj_set_style_transform_rotation(s_secondHand, secAngle,  0);
    lv_obj_set_style_transform_rotation(s_minuteHand, minAngle,  0);
    lv_obj_set_style_transform_rotation(s_hourHand,   hourAngle, 0);
}
