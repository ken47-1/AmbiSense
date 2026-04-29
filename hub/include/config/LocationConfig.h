/* ========== LocationConfig.h ========== */
/* Location settings for weather API */
#pragma once

/* ===== COORDINATES ===== */
constexpr float LOCATION_LAT  = 0.0f;
constexpr float LOCATION_LON  = 0.0f;

/* ===== TIMEZONE ===== */
constexpr long  LOCATION_GMT_OFFSET_SEC      = 0 * 3600;
constexpr int   LOCATION_DAYLIGHT_OFFSET_SEC = 0;

/* ===== DISPLAY NAME ===== */
constexpr const char* LOCATION_NAME = "CITY_NAME";
