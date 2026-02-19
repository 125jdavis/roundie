/**
 * unit_convert.h
 * Inline unit-conversion helpers used throughout the gauge screens.
 */

#pragma once

/**
 * Convert kilopascals to pounds-per-square-inch.
 * @param kpa  pressure in kPa
 * @return     pressure in psi
 */
inline float kPaToPsi(float kpa) {
    return kpa * 0.145038f;
}

/**
 * Convert degrees Celsius to degrees Fahrenheit.
 * @param c  temperature in °C
 * @return   temperature in °F
 */
inline float celsiusToFahrenheit(float c) {
    return (c * 1.8f) + 32.0f;
}

/**
 * Convert lambda (stoichiometric ratio) to Air-Fuel Ratio.
 * Uses gasoline stoichiometric ratio of 14.7:1.
 * @param lambda  dimensionless lambda value
 * @return        AFR value
 */
inline float lambdaToAFR(float lambda) {
    return lambda * 14.7f;
}
