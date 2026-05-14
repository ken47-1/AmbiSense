/* ==================== WeatherTypes.cpp ==================== */
#include "WeatherTypes.h"

/* =============== DATA =============== */
// NOTES ARE PARTLY COPIED FROM
// https://www.nodc.noaa.gov/archive/arc0021/0002199/1.1/data/0-data/HTML/WMO-CODE/WMO4677.HTM
static constexpr WeatherInfo WEATHER_TABLE[] = {
    {0,  "\xF3\xB0\x96\x99", "Clear sky", WeatherColors::SUN},

    {1,  "\xF3\xB0\x96\x95", "Mostly clear",  WeatherColors::CLOUD},
    {2,  "\xF3\xB0\x96\x95", "Partly cloudy", WeatherColors::LIGHT_GRAY},
    {3,  "\xF3\xB0\x96\x90", "Overcast",      WeatherColors::DARK_GRAY},

    {45, "\xF3\xB0\x96\x91", "Fog", WeatherColors::FOG},  // No appreciable change during the preceding hour
    {48, "\xF3\xB0\x96\x91", "Fog", WeatherColors::FOG},  // Has begun or has become thicker during the preceding hour

    {51, "\xF3\xB0\x96\x97", "Drizzle", WeatherColors::RAIN},  // Slight
    {53, "\xF3\xB0\x96\x97", "Drizzle", WeatherColors::RAIN},  // Moderate
    {55, "\xF3\xB0\x96\x97", "Drizzle", WeatherColors::RAIN},  // Heavy (dense)

    {61, "\xF3\xB0\x96\x96", "Rain", WeatherColors::RAIN},  // Slight
    {63, "\xF3\xB0\x96\x96", "Rain", WeatherColors::RAIN},  // Moderate
    {65, "\xF3\xB0\x96\x96", "Rain", WeatherColors::RAIN},  // Heavy

    {71, "\xF3\xB0\x96\x90", "Snow", WeatherColors::SNOW},  // Slight
    {73, "\xF3\xB0\x96\x90", "Snow", WeatherColors::SNOW},  // Moderate
    {75, "\xF3\xB0\x96\x90", "Snow", WeatherColors::SNOW},  // Heavy

    {80, "\xF3\xB0\x96\x97", "Rain showers", WeatherColors::RAIN},  // Slight
    {81, "\xF3\xB0\x96\x97", "Rain showers", WeatherColors::RAIN},  // Moderate or heavy
    {82, "\xF3\xB0\x96\x97", "Rain showers", WeatherColors::RAIN},  // Violent

    {95, "\xF3\xB0\x99\xBE", "Thunderstorm", WeatherColors::STORM},  // Slight or moderate, without hail
    {96, "\xF3\xB0\x99\xBE", "Thunderstorm", WeatherColors::STORM},  // Slgiht or moderate, with hail
    {97, "\xF3\xB0\x99\xBE", "Thunderstorm", WeatherColors::STORM},  // Heavy, without hail
    {99, "\xF3\xB0\x99\xBE", "Thunderstorm", WeatherColors::STORM},  // Heavy, with hail
};

/* =============== INTERNAL LOOKUP =============== */
static const int WEATHER_TABLE_SIZE =
    sizeof(WEATHER_TABLE) / sizeof(WEATHER_TABLE[0]);

static const WeatherInfo* findWeather(int code) {
    for (int i = 0; i < WEATHER_TABLE_SIZE; ++i) {
        if (WEATHER_TABLE[i].code == code)
            return &WEATHER_TABLE[i];
    }
    return nullptr;
}

/* =============== PUBLIC API =============== */
WeatherInfo getWeatherInfo(int code) {
    const WeatherInfo* w = findWeather(code);

    if (w) return *w;

    return UNKNOWN_WEATHER;
}

const char* getWindDirection(int deg) {
    static const char* dirs[] = {
        "North","NE","East","SE","South","SW","West","NW"
    };

    int norm = ((deg % 360) + 360) % 360;
    int idx = (norm + 22) / 45;
    idx &= 7;
    return dirs[idx];
}