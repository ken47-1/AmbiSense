/* ========== WeatherTypes.cpp ========== */
#include "WeatherTypes.h"

/* ===== PUBLIC API ===== */

const char* getWeatherIcon(int code) {
    switch (code) {
        case 0:  return "\xF3\xB0\x96\x99"; // Clear Sky (Sunny)
        case 1:
        case 2:  return "\xF3\xB0\x96\x95"; // Partly Cloudy (Partly Cloudy)
        case 3:  return "\xF3\xB0\x96\x90"; // Overcast (Cloudy)
        case 45:
        case 48: return "\xF3\xB0\x96\x91"; // Fog (Fog)
        case 51:
        case 53:
        case 55: return "\xF3\xB0\x96\x97"; // Drizzle (Rainy)
        case 61:
        case 63:
        case 65: return "\xF3\xB0\x96\x96"; // Rain (Pouring)
        case 71:
        case 73:
        case 75: return "\xF3\xB0\x96\x90"; // Snow (Defaulted to Cloudy - no snow icon listed)
        case 80:
        case 81:
        case 82: return "\xF3\xB0\x96\x97"; // Rain Showers (Rainy)
        case 95:
        case 96:
        case 99: return "\xF3\xB0\x99\xBE"; // Thunderstorm (Thunderstorm)
        default: return "\xF3\xB0\x8B\x97"; // Unknown (Question mark)
    }
}

const char* getWeatherDesc(int code) {
    switch (code) {
        case 0: return "Clear sky";
        case 1: return "Mostly clear";
        case 2: return "Partly cloudy";
        case 3: return "Overcast";
        case 45:
        case 48: return "Fog";
        case 51:
        case 53:
        case 55: return "Drizzle";
        case 61:
        case 63:
        case 65: return "Rain";
        default: return "Unknown";
    }
}

const char* getWindDir(int deg) {
    static const char* dirs[] = {
        "North","NE","East","SE","South","SW","West","NW"
    };

    int idx = (deg + 22) / 45;
    idx &= 7;
    return dirs[idx];
}