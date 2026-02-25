/**
 * roundie.ino
 * ESP32-S3 Automotive CAN Gauge Display
 *
 * Hardware: Waveshare ESP32-S3-Touch-AMOLED-1.75 (SKU 31261)
 *   Display:  1.75" round AMOLED 466×466, CO5300 (QSPI)
 *   Touch:    CST9217  (I2C – GPIO14/SCL, GPIO15/SDA)
 *   RTC:      PCF85063 (I2C – shared bus with touch)
 *   CAN:      MCP2515  (SPI – see config.h for pin assignments)
 *
 * Screens:
 *   0 – Analog clock  (PCF85063 RTC)
 *   1 – Multi-arc gauges (boost, lambda/AFR, fuel pressure)
 *   2 – Analog boost gauge (needle, 0–3 bar)
 *   3 – Setup screen (unit selection: Metric / 'Merican)
 *
 * Libraries required (install via Arduino Library Manager):
 *   - LVGL            ≥ 9.0  (lv_conf.h: LV_COLOR_DEPTH 16, LV_FONT_UNSCII_8, LV_FONT_UNSCII_16)
 *   - mcp2515         by autowp (https://github.com/autowp/arduino-mcp2515)
 *   - RTClib          by Adafruit
 *   - Waveshare BSP / TFT_eSPI configured for CO5300 QSPI display
 *       OR use the Waveshare-provided Arduino library for ESP32-S3-AMOLED-1.75
 *
 * Arduino IDE settings:
 *   Board:            ESP32S3 Dev Module
 *   Partition Scheme: Huge APP (3MB No OTA / 1MB SPIFFS) or 16MB Flash variants
 *   PSRAM:            OPI PSRAM
 *   Upload Speed:     921600
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * MCP2515 wiring (use the 8-pin expansion header GPIOs):
 *   MCP2515 MOSI  → GPIO 11    (CAN_SPI_MOSI in config.h)
 *   MCP2515 MISO  → GPIO 12    (CAN_SPI_MISO)
 *   MCP2515 SCK   → GPIO 13    (CAN_SPI_SCK)
 *   MCP2515 CS    → GPIO 10    (CAN_SPI_CS)
 *   MCP2515 INT   → GPIO  9    (CAN_INT_PIN, optional)
 *   MCP2515 VCC   → 3.3V
 *   MCP2515 GND   → GND
 *   Add 120Ω termination resistor between CANH and CANL externally.
 *
 * Setting the RTC time on first boot:
 *   Uncomment the lv_rtc.adjust() line in setup() below, upload once,
 *   then re-comment it and re-upload to avoid resetting time on every boot.
 * ─────────────────────────────────────────────────────────────────────────────
 */

// ── Arduino / ESP32 standard libraries ───────────────────────────────────────
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Preferences.h>

// ── LVGL ─────────────────────────────────────────────────────────────────────
#include <lvgl.h>
// lv_conf.h must enable:
//   LV_COLOR_DEPTH  16
//   LV_FONT_UNSCII_8, LV_FONT_UNSCII_16
//   LV_USE_ARC, LV_USE_SCALE
//   LV_USE_BTN, LV_USE_LABEL

// ── MCP2515 CAN controller ────────────────────────────────────────────────────
#include <mcp2515.h>

// ── PCF85063 RTC ──────────────────────────────────────────────────────────────
#include <RTClib.h>

// ── Waveshare display + touch driver ─────────────────────────────────────────
// Include the BSP header provided by Waveshare for the ESP32-S3-AMOLED-1.75.
// Adjust the include path to match your installed Waveshare library.
// Example: #include <ESP32_S3_Box.h>  or  #include <SWIRE_SH8601.h>
// The display should expose:
//   void display_init(void)
//   void display_flush(lv_display_t*, const lv_area_t*, uint8_t*)
//   void touch_read(lv_indev_t*, lv_indev_data_t*)
//
// *** PLACEHOLDER – replace with your actual Waveshare library include: ***
// #include <WaveshareAMOLED.h>

// ── Project headers ───────────────────────────────────────────────────────────
#include "config.h"
#include "unit_convert.h"
#include "can_handler.h"
#include "screen_clock.h"
#include "screen_multiarc.h"
#include "screen_boostgauge.h"
#include "screen_setup.h"
#include "gestures.h"

// ═══════════════════════════════════════════════════════════════════════════════
// Global variables
// ═══════════════════════════════════════════════════════════════════════════════

// ── Sensor data (defined here, declared extern in can_handler.h) ──────────────
float    g_lambda       = 1.0f;   // dimensionless
float    g_boostKpa     = 0.0f;   // kPa absolute
float    g_fuelPressKpa = 0.0f;   // kPa
uint16_t g_rpm          = 0;
float    g_coolantC     = 20.0f;  // °C
float    g_oilPressKpa  = 0.0f;   // kPa

// ── Unit system (true = Metric, false = Imperial/'Merican) ────────────────────
bool g_isMetric = true;

// ── Navigation state ──────────────────────────────────────────────────────────
int       g_currentScreen = SCREEN_CLOCK;
int       g_prevScreen    = SCREEN_CLOCK;
lv_obj_t *g_screens[4]   = {};   // indexed by SCREEN_xxx constants

// ── Peripheral objects ────────────────────────────────────────────────────────
static MCP2515 g_mcp2515(CAN_SPI_CS);
static RTC_PCF85063 g_rtc;
Preferences g_prefs;

// ── LVGL display buffer ───────────────────────────────────────────────────────
// Double-buffer DMA for smooth rendering on ESP32-S3 with PSRAM
// Each buffer holds 1/10 of the screen pixels – placed in PSRAM if available
static lv_color_t *s_buf1 = nullptr;
static lv_color_t *s_buf2 = nullptr;
#define DISP_BUF_LINES  46   // 466 / 10 ≈ 46 lines per buffer

static bool s_rtcReady = false;


#if CAN_INT_PIN >= 0
static volatile bool s_canMsgReady = false;
static void IRAM_ATTR _canIsr(void) { s_canMsgReady = true; }
#endif

// ═══════════════════════════════════════════════════════════════════════════════
// LVGL tick source
// ═══════════════════════════════════════════════════════════════════════════════

static void _lvTickTimerCb(void) {
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Display driver callbacks
// ── REPLACE with calls to your Waveshare BSP functions ───────────────────────
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * LVGL flush callback.
 * Transfers rendered pixels to the physical display.
 * *** Replace the body with your actual Waveshare CO5300 driver call. ***
 */
static void _displayFlush(lv_display_t *disp, const lv_area_t *area,
                           uint8_t *colorMap) {
    // Example for Waveshare BSP:
    //   waveshare_display_flush(area->x1, area->y1, area->x2, area->y2,
    //                           (uint16_t *)colorMap);
    //
    // Always call lv_display_flush_ready() when the transfer is complete:
    lv_display_flush_ready(disp);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Touch driver callback
// ── REPLACE with calls to your CST9217 / Waveshare BSP functions ─────────────
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * LVGL touch-read callback.
 * *** Replace the body with your actual CST9217 driver call. ***
 */
static void _touchRead(lv_indev_t *indev, lv_indev_data_t *data) {
    (void)indev;
    // Example for Waveshare BSP:
    //   uint16_t tx, ty;
    //   bool touched = waveshare_touch_read(&tx, &ty);
    //   if (touched) {
    //       data->point.x = tx;
    //       data->point.y = ty;
    //       data->state   = LV_INDEV_STATE_PR;
    //   } else {
    //       data->state   = LV_INDEV_STATE_REL;
    //   }
    data->state = LV_INDEV_STATE_REL;  // placeholder
}

// ═══════════════════════════════════════════════════════════════════════════════
// Screen switching
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Load a screen by index.  Handles setup-screen housekeeping.
 */
void switchToScreen(int idx) {
    if (idx < 0 || idx > SCREEN_SETUP) return;
    if (!g_screens[idx]) return;

    g_currentScreen = idx;
    lv_scr_load_anim(g_screens[idx], LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false);

    // Refresh setup highlight whenever we enter that screen
    if (idx == SCREEN_SETUP) {
        updateSetupScreen();
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// CAN message reception
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Poll the MCP2515 for pending messages and dispatch to parseCAN().
 * Called every loop iteration (or on interrupt flag).
 */
static void _readCAN(void) {
    struct can_frame frame;

    // Read up to 8 frames per call to avoid blocking the LVGL handler
    for (int i = 0; i < 8; i++) {
        if (g_mcp2515.readMessage(&frame) == MCP2515::ERROR_OK) {
            parseCAN(frame.can_id, frame.can_dlc, frame.data);
        } else {
            break;
        }
    }

#if CAN_INT_PIN >= 0
    s_canMsgReady = false;
#endif
}

// ═══════════════════════════════════════════════════════════════════════════════
// Setup
// ═══════════════════════════════════════════════════════════════════════════════

void setup(void) {
    Serial.begin(115200);
    Serial.println("[roundie] Booting…");

    // ── I2C (touch + RTC share the same bus) ──────────────────────────────
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    // ── RTC ───────────────────────────────────────────────────────────────
    if (!g_rtc.begin(&Wire)) {
        Serial.println("[RTC] PCF85063 not found – continuing without RTC");
    } else {
        s_rtcReady = true;
        if (!g_rtc.initialized() || g_rtc.lostPower()) {
            Serial.println("[RTC] Power loss detected – set time manually");
            // *** Uncomment and adjust the line below on first upload to set time:
            // g_rtc.adjust(DateTime(2025, 1, 1, 12, 0, 0));
        }
        Serial.println("[RTC] OK");
    }

    // ── MCP2515 SPI ───────────────────────────────────────────────────────
    SPI.begin(CAN_SPI_SCK, CAN_SPI_MISO, CAN_SPI_MOSI, CAN_SPI_CS);
    g_mcp2515.reset();
    if (g_mcp2515.setBitrate(CAN_SPEED, MCP_8MHZ) == MCP2515::ERROR_OK) {
        Serial.println("[CAN] Bitrate set");
    } else {
        Serial.println("[CAN] WARNING: setBitrate failed – check MCP2515 crystal");
    }

    // Accept only the three Haltech CAN V2 IDs we care about
    // MCP2515 mask/filter setup: use mask 0 (RXB0) for first two IDs,
    // mask 1 (RXB1) for the third.
    g_mcp2515.setFilterMask(MCP2515::MASK0, false, 0x7FF);
    g_mcp2515.setFilter(MCP2515::RXF0, false, CAN_ID_LAMBDA_BOOST_FUELPRES);
    g_mcp2515.setFilter(MCP2515::RXF1, false, CAN_ID_RPM);
    g_mcp2515.setFilterMask(MCP2515::MASK1, false, 0x7FF);
    g_mcp2515.setFilter(MCP2515::RXF2, false, CAN_ID_COOLANT_OILPRES);
    g_mcp2515.setFilter(MCP2515::RXF3, false, CAN_ID_COOLANT_OILPRES);
    g_mcp2515.setFilter(MCP2515::RXF4, false, CAN_ID_COOLANT_OILPRES);
    g_mcp2515.setFilter(MCP2515::RXF5, false, CAN_ID_COOLANT_OILPRES);

    g_mcp2515.setNormalMode();
    Serial.println("[CAN] MCP2515 ready");

#if CAN_INT_PIN >= 0
    pinMode(CAN_INT_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(CAN_INT_PIN), _canIsr, FALLING);
    Serial.println("[CAN] Interrupt-driven RX enabled");
#endif

    // ── Display + touch initialisation ───────────────────────────────────
    // *** Replace with your Waveshare BSP init call, e.g.:
    // waveshare_display_init();
    // waveshare_touch_init();
    Serial.println("[DISP] Display init (placeholder)");

    // ── LVGL initialisation ───────────────────────────────────────────────
    lv_init();

    // Allocate draw buffers in PSRAM
    s_buf1 = (lv_color_t *)heap_caps_malloc(
        DISPLAY_WIDTH * DISP_BUF_LINES * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    s_buf2 = (lv_color_t *)heap_caps_malloc(
        DISPLAY_WIDTH * DISP_BUF_LINES * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    if (!s_buf1 || !s_buf2) {
        // Fallback to internal RAM with a smaller buffer
        static lv_color_t fallbackBuf1[DISPLAY_WIDTH * 10];
        static lv_color_t fallbackBuf2[DISPLAY_WIDTH * 10];
        s_buf1 = fallbackBuf1;
        s_buf2 = fallbackBuf2;
        Serial.println("[LVGL] PSRAM unavailable – using internal RAM buffer");
    }
    lv_display_t *disp = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_display_set_flush_cb(disp, _displayFlush);
    lv_display_set_buffers(disp, s_buf1, s_buf2,
                           DISPLAY_WIDTH * DISP_BUF_LINES * sizeof(lv_color_t),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Register touch input device
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, _touchRead);
    // Enable gesture detection
    lv_indev_set_gesture_limit(indev, 20);   // minimum gesture distance in px

    // LVGL tick source via hardware timer (ESP32-S3)
    // Use a 1-kHz hardware timer to call lv_tick_inc every LV_TICK_PERIOD_MS ms
    static hw_timer_t *lvTimer = nullptr;
    lvTimer = timerBegin(0, 80, true);   // timer 0, prescaler 80 → 1 µs/tick
    timerAttachInterrupt(lvTimer, _lvTickTimerCb, true);
    timerAlarmWrite(lvTimer, LV_TICK_PERIOD_MS * 1000UL, true);
    timerAlarmEnable(lvTimer);

    // ── NVS: load saved unit preference ──────────────────────────────────
    g_prefs.begin(NVS_NAMESPACE, false);
    g_isMetric = g_prefs.getBool(NVS_KEY_IS_METRIC, true);  // default: Metric
    Serial.printf("[NVS] isMetric = %s\n", g_isMetric ? "true" : "false");

    // ── Create all LVGL screens ───────────────────────────────────────────
    g_screens[SCREEN_CLOCK]      = createClockScreen();
    g_screens[SCREEN_MULTIARC]   = createMultiArcScreen();
    g_screens[SCREEN_BOOSTGAUGE] = createAnalogBoostScreen();
    g_screens[SCREEN_SETUP]      = createSetupScreen();

    // ── Install gesture/long-press handlers ───────────────────────────────
    installGestureHandlers();

    // ── Load default screen ───────────────────────────────────────────────
    switchToScreen(SCREEN_CLOCK);

    Serial.println("[roundie] Setup complete");
}

// ═══════════════════════════════════════════════════════════════════════════════
// Loop
// ═══════════════════════════════════════════════════════════════════════════════

void loop(void) {
    // ── LVGL task handler ─────────────────────────────────────────────────
    lv_timer_handler();

    // ── CAN message processing ────────────────────────────────────────────
#if CAN_INT_PIN >= 0
    if (s_canMsgReady) {
        _readCAN();
    }
#else
    _readCAN();
#endif

    // ── Per-screen UI updates ─────────────────────────────────────────────
    static uint32_t lastUpdateMs = 0;
    uint32_t now = millis();
    if (now - lastUpdateMs >= 100) {   // update UI at ~10 Hz
        lastUpdateMs = now;

        switch (g_currentScreen) {
            case SCREEN_CLOCK: {
                // Read time from RTC (only if initialised successfully)
                if (s_rtcReady) {
                    DateTime t = g_rtc.now();
                    updateClockScreen(t.hour(), t.minute(), t.second());
                }
                break;
            }
            case SCREEN_MULTIARC:
                updateMultiArcScreen();
                break;
            case SCREEN_BOOSTGAUGE:
                updateAnalogBoostScreen();
                break;
            case SCREEN_SETUP:
                // No continuous update needed; changes are event-driven
                break;
        }
    }

    // Small yield to keep watchdog happy
    delay(1);
}
