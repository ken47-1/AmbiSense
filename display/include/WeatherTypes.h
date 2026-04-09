/* ========== WeatherTypes.h ========== */
#pragma once

/* ===== TYPES ===== */

struct WeatherData {
    bool  valid;

    float temp;
    float feelsLike;

    int   humidity;
    int   pressure;

    float windSpeed;
    int   windDirection;

    char  sunrise[6];   // "HH:MM"
    char  sunset[6];

    int   weatherCode;
};

/* ===== API ===== */

const char* getWeatherIcon(int code);
const char* getWeatherDesc(int code);
const char* getWindDir(int deg);