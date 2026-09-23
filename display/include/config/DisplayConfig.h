/* ==================== DisplayConfig.h ==================== */
/* General configuration for AmbiSense Display */
#pragma once

/* =============== INCLUDES =============== */
/* ============ CORE ============ */
#include <stdint.h>

/* =============== DISPLAY BRIGHTNESS =============== */
constexpr uint8_t BRIGHTNESS_MIN_PERCENT = 5;
constexpr uint8_t BRIGHTNESS_MAX_PERCENT = 100;

/* =============== INTERVALS =============== */
constexpr uint32_t SCAN_HOP_INTERVAL_MS   = 1000;  // Channel scan rate

/* =============== TIMEOUTS =============== */
constexpr uint32_t HUB_OFFLINE_TIMEOUT_MS    = 15000;  // Hub lost detection
constexpr uint32_t STALE_DATA_TIMEOUT_MS     = 5000;   // Weather/room expiry
