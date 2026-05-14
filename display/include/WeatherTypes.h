/* ==================== WeatherTypes.h ==================== */
#pragma once

/* =============== INCLUDES =============== */
/* ============ PROJECT ============ */
#include "WeatherColors.h"

/* =============== TYPES =============== */
/* ============ STRUCTS ============ */
struct WeatherData {
    bool  valid;
    float temp;
    float feelsLike;
    int   humidity;
    int   pressure;
    float windSpeed;
    int   windDirection;
    char  sunrise[6];
    char  sunset[6];
    int   weatherCode;
};

struct WeatherInfo {
    int         code;
    const char* icon;
    const char* desc;
    uint32_t    color;
};

/* =============== UNKNOWN WEATHER =============== */
static constexpr WeatherInfo UNKNOWN_WEATHER = {
    -1,
    "\xF3\xB0\x8B\x97",
    "Unknown",
    WeatherColors::DARK_GRAY
};

/* =============== API =============== */
WeatherInfo getWeatherInfo(int code);
const char* getWindDirection(int deg);