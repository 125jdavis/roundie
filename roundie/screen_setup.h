/**
 * screen_setup.h
 * Screen 3 – Unit-system setup screen.
 *
 * Access:  3-second press-and-hold on any main screen
 * Exit:    Swipe down  OR  3-second press-and-hold again
 *
 * UI:
 *   Two option buttons: "Metric" and "'Merican"
 *   Current selection highlighted with an orange outline/text
 *   Swipe left/right or tap to toggle selection
 *   Selection is saved to NVS on change
 *
 * Background: Black   Text: White   Highlight: Orange
 */

#pragma once

#include <lvgl.h>
#include <Preferences.h>
#include "config.h"

extern bool g_isMetric;
extern Preferences g_prefs;

// ── Setup screen widget handles ───────────────────────────────────────────────
static lv_obj_t *s_setupScreen     = nullptr;
static lv_obj_t *s_btnMetric       = nullptr;
static lv_obj_t *s_btnMerican      = nullptr;
static lv_obj_t *s_lblSetupTitle   = nullptr;
static lv_obj_t *s_lblSetupHint    = nullptr;

static lv_style_t s_styleBtnNormal;
static lv_style_t s_styleBtnSelected;

// Forward declaration
static void _applySetupSelection(bool isMetric);

// ── Button event callbacks ────────────────────────────────────────────────────
static void _onMetricTapped(lv_event_t *e) {
    (void)e;
    if (!g_isMetric) {
        g_isMetric = true;
        g_prefs.putBool(NVS_KEY_IS_METRIC, true);
        _applySetupSelection(true);
    }
}

static void _onMericanTapped(lv_event_t *e) {
    (void)e;
    if (g_isMetric) {
        g_isMetric = false;
        g_prefs.putBool(NVS_KEY_IS_METRIC, false);
        _applySetupSelection(false);
    }
}

/**
 * Apply visual selection highlight to the currently active unit button.
 */
static void _applySetupSelection(bool isMetric) {
    if (!s_btnMetric || !s_btnMerican) return;

    if (isMetric) {
        lv_obj_add_style(s_btnMetric,  &s_styleBtnSelected, 0);
        lv_obj_remove_style(s_btnMerican, &s_styleBtnSelected, 0);
    } else {
        lv_obj_add_style(s_btnMerican, &s_styleBtnSelected, 0);
        lv_obj_remove_style(s_btnMetric, &s_styleBtnSelected, 0);
    }
}

/**
 * Create all LVGL widgets for the setup screen.
 * @return pointer to the screen object
 */
static lv_obj_t *createSetupScreen(void) {
    // ── Styles ─────────────────────────────────────────────────────────────
    lv_style_init(&s_styleBtnNormal);
    lv_style_set_bg_color(&s_styleBtnNormal, lv_color_make(0x20, 0x20, 0x20));
    lv_style_set_bg_opa(&s_styleBtnNormal, LV_OPA_COVER);
    lv_style_set_border_color(&s_styleBtnNormal, lv_color_make(0x60, 0x60, 0x60));
    lv_style_set_border_width(&s_styleBtnNormal, 2);
    lv_style_set_radius(&s_styleBtnNormal, 12);
    lv_style_set_text_color(&s_styleBtnNormal, lv_color_white());

    lv_style_init(&s_styleBtnSelected);
    lv_style_set_border_color(&s_styleBtnSelected, lv_color_make(0xFF, 0x80, 0x00)); // orange
    lv_style_set_border_width(&s_styleBtnSelected, 4);
    lv_style_set_text_color(&s_styleBtnSelected, lv_color_make(0xFF, 0x80, 0x00));

    // ── Screen ─────────────────────────────────────────────────────────────
    s_setupScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(s_setupScreen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_setupScreen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_setupScreen, LV_OBJ_FLAG_SCROLLABLE);

    // Title label
    s_lblSetupTitle = lv_label_create(s_setupScreen);
    lv_label_set_text(s_lblSetupTitle, "Units");
    lv_obj_set_style_text_color(s_lblSetupTitle, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_lblSetupTitle, &lv_font_unscii_16, 0);
    lv_obj_align(s_lblSetupTitle, LV_ALIGN_TOP_MID, 0, 60);

    // "Metric" button
    s_btnMetric = lv_btn_create(s_setupScreen);
    lv_obj_set_size(s_btnMetric, 180, 80);
    lv_obj_align(s_btnMetric, LV_ALIGN_CENTER, -100, 10);
    lv_obj_add_style(s_btnMetric, &s_styleBtnNormal, 0);
    lv_obj_add_event_cb(s_btnMetric, _onMetricTapped, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *lblMetric = lv_label_create(s_btnMetric);
    lv_label_set_text(lblMetric, "Metric");
    lv_obj_set_style_text_font(lblMetric, &lv_font_unscii_16, 0);
    lv_obj_center(lblMetric);

    // "'Merican" button
    s_btnMerican = lv_btn_create(s_setupScreen);
    lv_obj_set_size(s_btnMerican, 180, 80);
    lv_obj_align(s_btnMerican, LV_ALIGN_CENTER, 100, 10);
    lv_obj_add_style(s_btnMerican, &s_styleBtnNormal, 0);
    lv_obj_add_event_cb(s_btnMerican, _onMericanTapped, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *lblMerican = lv_label_create(s_btnMerican);
    lv_label_set_text(lblMerican, "'Merican");
    lv_obj_set_style_text_font(lblMerican, &lv_font_unscii_16, 0);
    lv_obj_center(lblMerican);

    // Hint label
    s_lblSetupHint = lv_label_create(s_setupScreen);
    lv_label_set_text(s_lblSetupHint, "Swipe down or hold 3s to exit");
    lv_obj_set_style_text_color(s_lblSetupHint, lv_color_make(0x80, 0x80, 0x80), 0);
    lv_obj_set_style_text_font(s_lblSetupHint, &lv_font_unscii_8, 0);
    lv_obj_align(s_lblSetupHint, LV_ALIGN_BOTTOM_MID, 0, -50);

    // Apply initial selection highlight
    _applySetupSelection(g_isMetric);

    return s_setupScreen;
}

/**
 * Refresh the setup-screen highlight to match the current g_isMetric value.
 * Call whenever this screen becomes active.
 */
static void updateSetupScreen(void) {
    _applySetupSelection(g_isMetric);
}
