/**
 * config.h
 * Hardware pin definitions and global configuration for the
 * Waveshare ESP32-S3-Touch-AMOLED-1.75 CAN gauge project.
 *
 * Wiring – MCP2515 to ESP32-S3 8-pin expansion header:
 *   MCP2515 MOSI  → GPIO 11
 *   MCP2515 MISO  → GPIO 12
 *   MCP2515 SCK   → GPIO 13
 *   MCP2515 CS    → GPIO 10
 *   MCP2515 INT   → GPIO 9  (optional interrupt-driven RX)
 *   MCP2515 VCC   → 3.3V
 *   MCP2515 GND   → GND
 *   (Add 120Ω termination resistor on CAN bus externally)
 *
 * Display – CO5300 AMOLED (QSPI, managed by Waveshare BSP):
 *   CLK  → GPIO 47
 *   MOSI → GPIO 18
 *   CS   → GPIO 38
 *   DC   → GPIO 7
 *   RST  → GPIO 17
 *
 * Touch – CST9217 (I2C):
 *   SCL  → GPIO 14
 *   SDA  → GPIO 15
 *
 * RTC – PCF85063 (I2C, shares bus with touch):
 *   SCL  → GPIO 14
 *   SDA  → GPIO 15
 */

#pragma once

// ── Display ──────────────────────────────────────────────────────────────────
#define DISPLAY_WIDTH   466
#define DISPLAY_HEIGHT  466

// ── MCP2515 SPI pins ─────────────────────────────────────────────────────────
#define CAN_SPI_MOSI    11
#define CAN_SPI_MISO    12
#define CAN_SPI_SCK     13
#define CAN_SPI_CS      10
#define CAN_INT_PIN      9   // Set to -1 to disable interrupt-driven RX

// ── Touch / RTC I2C ──────────────────────────────────────────────────────────
#define I2C_SCL_PIN     14
#define I2C_SDA_PIN     15

// ── CAN configuration ────────────────────────────────────────────────────────
// Haltech CAN V2 runs at 1 Mbps; change CAN_SPEED if your setup differs.
#define CAN_SPEED       CAN_1000KBPS

// ── Haltech CAN V2 message IDs ───────────────────────────────────────────────
#define CAN_ID_LAMBDA_BOOST_FUELPRES    0x3D0
#define CAN_ID_RPM                      0x3D1
#define CAN_ID_COOLANT_OILPRES          0x3D2

// ── LVGL tick interval ───────────────────────────────────────────────────────
#define LV_TICK_PERIOD_MS   5   // ms between lv_tick_inc() calls

// ── Gesture / long-press timing ──────────────────────────────────────────────
#define LONG_PRESS_MS       3000  // 3-second hold to enter/exit setup screen

// ── NVS storage key ──────────────────────────────────────────────────────────
#define NVS_NAMESPACE       "roundie"
#define NVS_KEY_IS_METRIC   "isMetric"

// ── Screen indices ───────────────────────────────────────────────────────────
#define SCREEN_CLOCK        0
#define SCREEN_MULTIARC     1
#define SCREEN_BOOSTGAUGE   2
#define SCREEN_SETUP        3
#define SCREEN_COUNT        3   // number of main (swipeable) screens
