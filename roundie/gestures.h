/**
 * gestures.h
 * Touch gesture and long-press handling for screen navigation.
 *
 * Navigation rules:
 *   Swipe left  → advance to next main screen  (0 → 1 → 2 → 0)
 *   Swipe right → go to previous main screen   (0 → 2 → 1 → 0)
 *   Hold 3 s    → enter setup screen (from any main screen) or exit it
 *   Swipe down  → exit setup screen (returns to last main screen)
 *
 * The gesture callbacks are attached to each screen object in
 * installGestureHandlers().  A software timer tracks long-press duration.
 */

#pragma once

#include <lvgl.h>
#include "config.h"

// ── Navigation state (extern, defined in roundie.ino) ────────────────────────
extern int      g_currentScreen;   // 0-2 for main screens, 3 = setup
extern int      g_prevScreen;      // screen we came from before entering setup
extern lv_obj_t *g_screens[];      // array of screen objects [0..3]

// Forward declaration for the screen-switch function defined in roundie.ino
extern void switchToScreen(int idx);

// ── Long-press tracking ───────────────────────────────────────────────────────
static lv_timer_t  *s_longPressTimer   = nullptr;
static bool         s_longPressActive  = false;
static uint32_t     s_pressStartMs     = 0;

// ── Long-press timer callback ─────────────────────────────────────────────────
static void _longPressTimerCb(lv_timer_t *timer) {
    (void)timer;
    if (!s_longPressActive) return;

    uint32_t elapsed = lv_tick_get() - s_pressStartMs;
    if (elapsed >= LONG_PRESS_MS) {
        s_longPressActive = false;
        lv_timer_pause(s_longPressTimer);

        if (g_currentScreen == SCREEN_SETUP) {
            // Exit setup: return to previous main screen
            switchToScreen(g_prevScreen);
        } else {
            // Enter setup
            g_prevScreen = g_currentScreen;
            switchToScreen(SCREEN_SETUP);
        }
    }
}

// ── Generic screen event callback ─────────────────────────────────────────────
static void _screenEventCb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSING) {
        if (!s_longPressActive) {
            s_longPressActive = true;
            s_pressStartMs    = lv_tick_get();
            if (s_longPressTimer) {
                lv_timer_resume(s_longPressTimer);
                lv_timer_reset(s_longPressTimer);
            }
        }
        return;
    }

    // Touch released – cancel long-press tracking
    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        s_longPressActive = false;
        if (s_longPressTimer) lv_timer_pause(s_longPressTimer);
        return;
    }

    // Gesture events
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());

        if (g_currentScreen == SCREEN_SETUP) {
            // Only swipe-down exits setup
            if (dir == LV_DIR_BOTTOM) {
                switchToScreen(g_prevScreen);
            }
            return;
        }

        // Main screen navigation
        if (dir == LV_DIR_LEFT) {
            int next = (g_currentScreen + 1) % SCREEN_COUNT;
            switchToScreen(next);
        } else if (dir == LV_DIR_RIGHT) {
            int prev = (g_currentScreen - 1 + SCREEN_COUNT) % SCREEN_COUNT;
            switchToScreen(prev);
        }
    }
}

/**
 * Create the long-press timer and attach gesture/press callbacks to every
 * screen object in g_screens[].  Call once after all screens are created.
 */
static void installGestureHandlers(void) {
    // Create the long-press polling timer (paused initially)
    s_longPressTimer = lv_timer_create(_longPressTimerCb, 50, nullptr);
    lv_timer_pause(s_longPressTimer);

    // Attach event callbacks to all screens
    for (int i = 0; i <= SCREEN_SETUP; i++) {
        if (!g_screens[i]) continue;
        lv_obj_clear_flag(g_screens[i], LV_OBJ_FLAG_GESTURE_BUBBLE);
        lv_obj_add_event_cb(g_screens[i], _screenEventCb, LV_EVENT_PRESSING,    nullptr);
        lv_obj_add_event_cb(g_screens[i], _screenEventCb, LV_EVENT_RELEASED,    nullptr);
        lv_obj_add_event_cb(g_screens[i], _screenEventCb, LV_EVENT_PRESS_LOST,  nullptr);
        lv_obj_add_event_cb(g_screens[i], _screenEventCb, LV_EVENT_GESTURE,     nullptr);
    }
}
