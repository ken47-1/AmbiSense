/* ==================== HubConfig.h ==================== */
/* General configuration for AmbiSense Hub */
#pragma once

/* =============== INCLUDES =============== */
/* ============ CORE ============ */
#include <stdint.h>

/* =============== TIME =============== */
constexpr long     GMT_OFFSET_SEC            = 7 * 3600;        // UTC+7
constexpr int32_t  DAYLIGHT_OFFSET_SEC       = 0;               // No DST
constexpr int32_t  TARGET_SYNC_HOUR          = 12;              // Daily NTP sync hour
constexpr int32_t  TARGET_SYNC_MINUTE        = 0;               // Daily NTP sync minute
constexpr int32_t  NTP_SYNC_TIMEOUT_DELAY_MS = 10000;           // Per-attempt timeout
constexpr int32_t  NTP_SYNC_MAX_RETRIES      = 6;               // Max sync attempts

/* =============== WEATHER =============== */
constexpr uint32_t WEATHER_INTERVAL_MS      = 30 * 60 * 1000;   // Fetch interval
constexpr uint32_t WEATHER_RETRY_DELAY_MS   = 2000;             // Retry backoff base
constexpr uint8_t  WEATHER_MAX_RETRIES      = 3;                // Max fetch retries

/* =============== LOCATION =============== */
constexpr uint32_t LOCATION_RETRY_INTERVAL_MS = 5000;           // Retry interval
constexpr uint32_t LOCATION_MAX_RETRIES       = 10;             // Max retries

/* =============== INTERVALS =============== */
constexpr uint32_t DHT_INTERVAL_MS        = 2000;               // DHT22 poll rate
constexpr uint32_t BROADCAST_INTERVAL_MS  = 250;                // ESP-NOW rate
constexpr uint32_t SYNC_CHECK_INTERVAL_MS = 1000;               // RTC sync check rate

/* =============== TIMEOUTS =============== */
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS   = 15000;           // WiFi connection timeout
constexpr uint32_t WIFI_RETRY_INTERVAL_MS    = 30000;           // WiFi retry backoff