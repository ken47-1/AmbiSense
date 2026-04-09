/* ========== LocationConfig.h ========== */
/* Location settings for weather API */
#pragma once

/* ===== COORDINATES ===== */
constexpr float LOCATION_LAT  =  0.000000f;
constexpr float LOCATION_LON  = 0.000000f;

/* ===== TIMEZONE ===== */
constexpr long  LOCATION_GMT_OFFSET_SEC      = 7 * 3600;  // UTC+7, Bangkok
constexpr int   LOCATION_DAYLIGHT_OFFSET_SEC = 0;         // No DST in Thailand

/* ===== DISPLAY NAME ===== */
constexpr const char* LOCATION_NAME = "City_name";
