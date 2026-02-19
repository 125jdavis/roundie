/**
 * can_handler.h
 * CAN bus message parsing for Haltech CAN V2 protocol.
 *
 * All multi-byte values are little-endian (LSB first).
 *
 * Message layout:
 *   0x3D0 (8 bytes):
 *     Bytes 0-1  uint16 LE  Lambda          raw × 0.001  → Lambda value
 *     Bytes 2-3  int16  LE  Boost Pressure  raw × 0.1    → kPa absolute
 *     Bytes 4-5  int16  LE  Fuel Pressure   raw × 0.1    → kPa
 *
 *   0x3D1 (8 bytes):
 *     Bytes 0-1  uint16 LE  RPM             raw (direct) → RPM
 *
 *   0x3D2 (8 bytes):
 *     Bytes 0-1  int16  LE  Coolant Temp    raw × 0.1    → °C
 *     Bytes 2-3  int16  LE  Oil Pressure    raw × 0.1    → kPa
 */

#pragma once

#include <Arduino.h>
#include "config.h"

// ── Live sensor data (updated by parseCAN) ───────────────────────────────────
extern float g_lambda;          // dimensionless lambda (e.g. 1.0)
extern float g_boostKpa;        // kPa absolute
extern float g_fuelPressKpa;    // kPa
extern uint16_t g_rpm;          // RPM
extern float g_coolantC;        // °C
extern float g_oilPressKpa;     // kPa

/**
 * Parse a raw CAN frame and update the global sensor variables.
 *
 * @param id    11-bit CAN identifier
 * @param len   number of data bytes (DLC)
 * @param data  pointer to the data bytes (little-endian)
 */
inline void parseCAN(uint32_t id, uint8_t len, const uint8_t *data) {
    switch (id) {
        case CAN_ID_LAMBDA_BOOST_FUELPRES: {
            if (len < 6) break;
            // Lambda: bytes 0-1, uint16 LE, scale × 0.001
            uint16_t rawLambda = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
            g_lambda = rawLambda * 0.001f;

            // Boost pressure: bytes 2-3, int16 LE, scale × 0.1 → kPa absolute
            int16_t rawBoost = (int16_t)((uint16_t)data[2] | ((uint16_t)data[3] << 8));
            g_boostKpa = rawBoost * 0.1f;

            // Fuel pressure: bytes 4-5, int16 LE, scale × 0.1 → kPa
            int16_t rawFuel = (int16_t)((uint16_t)data[4] | ((uint16_t)data[5] << 8));
            g_fuelPressKpa = rawFuel * 0.1f;
            break;
        }

        case CAN_ID_RPM: {
            if (len < 2) break;
            // RPM: bytes 0-1, uint16 LE, direct value
            g_rpm = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
            break;
        }

        case CAN_ID_COOLANT_OILPRES: {
            if (len < 4) break;
            // Coolant temp: bytes 0-1, int16 LE, scale × 0.1 → °C
            int16_t rawCoolant = (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
            g_coolantC = rawCoolant * 0.1f;

            // Oil pressure: bytes 2-3, int16 LE, scale × 0.1 → kPa
            int16_t rawOil = (int16_t)((uint16_t)data[2] | ((uint16_t)data[3] << 8));
            g_oilPressKpa = rawOil * 0.1f;
            break;
        }

        default:
            break;
    }
}
