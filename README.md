# roundie

Round digital display for automotive applications, based on CAN bus.

## Hardware

| Component | Details |
|-----------|---------|
| Board | Waveshare ESP32-S3-Touch-AMOLED-1.75 (SKU 31261) |
| Display | 1.75" round AMOLED 466×466 px, CO5300 QSPI driver |
| Touch | CST9217 (I2C) |
| RTC | PCF85063 (I2C, shared bus with touch) |
| CAN | MCP2515 module (SPI via 8-pin expansion header) |
| PSRAM | 8 MB OPI PSRAM |
| Flash | 16 MB |

## Screens

| # | Screen | Description |
|---|--------|-------------|
| 0 | **Analog Clock** | Hour/minute/second orange hands, white tick marks & numerals, sourced from PCF85063 RTC |
| 1 | **Multi-Arc Gauge** | Outer arc = boost (0–300 kPa / 0–43.5 psi), inner arc = lambda/AFR (blue; red when lean under boost), center digital boost readout, bottom arc = fuel pressure |
| 2 | **Analog Boost Gauge** | Traditional needle gauge 0–3 bar / 0–43.5 psi with major/minor tick marks |
| 3 | **Setup** | Toggle between **Metric** (kPa, °C, λ, bar) and **'Merican** (psi, °F, AFR); saved to NVS |

## Navigation

| Gesture | Action |
|---------|--------|
| Swipe left | Next main screen (0 → 1 → 2 → 0) |
| Swipe right | Previous main screen |
| Hold 3 s | Enter / exit Setup screen |
| Swipe down | Exit Setup screen |

## CAN Bus (Haltech CAN V2)

| Parameter | CAN ID | Bytes | Format | Formula |
|-----------|--------|-------|--------|---------|
| Lambda | 0x3D0 | 0–1 | uint16 LE | λ = raw × 0.001 |
| Boost pressure | 0x3D0 | 2–3 | int16 LE | kPa = raw × 0.1 |
| Fuel pressure | 0x3D0 | 4–5 | int16 LE | kPa = raw × 0.1 |
| RPM | 0x3D1 | 0–1 | uint16 LE | RPM = raw |
| Coolant temp | 0x3D2 | 0–1 | int16 LE | °C = raw × 0.1 |
| Oil pressure | 0x3D2 | 2–3 | int16 LE | kPa = raw × 0.1 |

Baud rate: **1 Mbps**. Add a 120 Ω termination resistor between CANH and CANL externally.

## Wiring – MCP2515 to ESP32-S3 Expansion Header

| MCP2515 pin | ESP32-S3 GPIO |
|-------------|---------------|
| MOSI | GPIO 11 |
| MISO | GPIO 12 |
| SCK | GPIO 13 |
| CS | GPIO 10 |
| INT | GPIO 9 (optional interrupt-driven RX) |
| VCC | 3.3 V |
| GND | GND |

> **Note:** The MCP2515 module must operate at 3.3 V logic. Most modules use a 5 V crystal oscillator; verify or use a level-shifter.

## Required Libraries

Install all via **Arduino Library Manager** or PlatformIO:

| Library | Version | Notes |
|---------|---------|-------|
| [LVGL](https://github.com/lvgl/lvgl) | ≥ 8.3 | Enable `LV_COLOR_DEPTH 16`, Montserrat fonts (14, 20, 24, 28, 48), `LV_USE_ARC`, `LV_USE_METER`/`LV_USE_SCALE`, `LV_USE_BTN`, `LV_USE_LABEL` in `lv_conf.h` |
| [mcp2515 by autowp](https://github.com/autowp/arduino-mcp2515) | latest | CAN controller |
| [RTClib by Adafruit](https://github.com/adafruit/RTClib) | ≥ 2.1 | PCF85063 RTC |
| Waveshare BSP for ESP32-S3-AMOLED-1.75 | — | Display & touch driver; see [Waveshare Wiki](https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75) |

## Arduino IDE Configuration

| Setting | Value |
|---------|-------|
| Board | ESP32S3 Dev Module |
| Partition Scheme | Huge APP (3 MB No OTA / 1 MB SPIFFS) |
| PSRAM | OPI PSRAM |
| CPU Frequency | 240 MHz |
| Upload Speed | 921600 |

## Setting the RTC Time

On **first upload only**:
1. Open `roundie.ino`
2. Find the commented line: `// g_rtc.adjust(DateTime(2025, 1, 1, 12, 0, 0));`
3. Uncomment it and set the correct date/time
4. Upload the sketch
5. Re-comment the line and upload again to prevent resetting time on every reboot

## File Structure

```
roundie/
├── roundie.ino           Main sketch (setup, loop, display/touch init)
├── config.h              Pin definitions, CAN IDs, constants
├── can_handler.h         Haltech CAN V2 message parsing
├── unit_convert.h        Metric ↔ Imperial conversion helpers
├── screen_clock.h        Screen 0 – analog clock
├── screen_multiarc.h     Screen 1 – multi-arc gauge
├── screen_boostgauge.h   Screen 2 – analog boost gauge
├── screen_setup.h        Screen 3 – unit selection setup
└── gestures.h            Swipe / long-press navigation
```

## Integrating the Waveshare Display Driver

The sketch contains placeholder comments wherever the display and touch BSP
must be called.  Replace the `_displayFlush()` and `_touchRead()` callbacks in
`roundie.ino` with calls to the Waveshare-provided functions for the CO5300
AMOLED display and CST9217 touch controller.  Refer to the
[Waveshare ESP32-S3-Touch-AMOLED-1.75 wiki](https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75)
for the correct driver library and example code.

## Lambda Warning Logic

The inner arc on Screen 1 turns **red** when:
- Boost > 120 kPa absolute **AND**
- Lambda > 1.1 (AFR > 16.17)

This indicates a lean condition under boost — a potentially damaging engine state.

## Unit Preference

The selected unit system (Metric / 'Merican) is stored in **NVS** and persists
across reboots. It defaults to **Metric** on the very first boot.
