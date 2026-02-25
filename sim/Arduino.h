/**
 * sim/Arduino.h
 * Minimal Arduino.h stub for the PC simulator.
 *
 * can_handler.h includes <Arduino.h> for basic integer types.  On the
 * simulator these types come from the standard C library; this stub just
 * re-exports them so the include succeeds without an Arduino toolchain.
 */

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
