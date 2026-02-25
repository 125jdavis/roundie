/**
 * sim/config.h
 * Simulator-only configuration constants.
 *
 * Mirrors the subset of roundie/config.h that the screen headers and
 * can_handler.h actually need on the PC simulator.  Hardware-only
 * definitions (SPI pins, I2C pins, CAN bitrate) are intentionally omitted
 * so that no embedded-only libraries are required to compile the sim.
 */

#pragma once

// ── Display ──────────────────────────────────────────────────────────────────
#define DISPLAY_WIDTH   466
#define DISPLAY_HEIGHT  466

// ── LVGL tick interval ───────────────────────────────────────────────────────
#define LV_TICK_PERIOD_MS   5

// ── Gesture / long-press timing ──────────────────────────────────────────────
#define LONG_PRESS_MS       3000

// ── NVS storage keys ─────────────────────────────────────────────────────────
#define NVS_NAMESPACE       "roundie"
#define NVS_KEY_IS_METRIC   "isMetric"

// ── Screen indices ───────────────────────────────────────────────────────────
#define SCREEN_CLOCK        0
#define SCREEN_MULTIARC     1
#define SCREEN_BOOSTGAUGE   2
#define SCREEN_SETUP        3
#define SCREEN_COUNT        3

// ── Haltech CAN V2 message IDs (needed by can_handler.h parseCAN) ─────────────
#define CAN_ID_LAMBDA_BOOST_FUELPRES    0x3D0
#define CAN_ID_RPM                      0x3D1
#define CAN_ID_COOLANT_OILPRES          0x3D2
