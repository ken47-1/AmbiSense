/* ==================== LocationResolver.cpp ==================== */
#include "Locationresolver.h"

/* =============== INCLUDES =============== */
/* ============ PROJECT ============ */
#include "config/LocationConfig.h"

/* ============ THIRD-PARTY ============ */
/* None */

/* ============ CORE ============ */
#include <Arduino.h>

/* =============== PUBLIC API =============== */
/* ============ LIFECYCLE ============ */
LocationResolver::LocationResolver() : _valid(false) {}

void LocationResolver::begin() {
    _cachedCity = _fetchFromAPI(LOCATION_LAT, LOCATION_LON);
    _valid = (_cachedCity != "Unknown");
    if (_valid) {
        Serial.printf("[GEO] Location: %s\n", _cachedCity.c_str());
    } else {
        Serial.println("[GEO] Location lookup failed");
    }
}

/* ============ ACTIONS ============ */
String LocationResolver::getCityName(float lat, float lon) {
    if (_valid && _cachedCity.length() > 0) {
        return _cachedCity;
    }
    return _fetchFromAPI(lat, lon);
}

/* =============== INTERNAL HELPERS =============== */
String LocationResolver::_fetchFromAPI(float lat, float lon) {
    HTTPClient http;
    char url[256];

    snprintf(url, sizeof(url),
        "https://nominatim.openstreetmap.org/reverse"
        "?lat=%.6f&lon=%.6f"
        "&format=json&zoom=10&addressdetails=1"
        "&accept-language=en",
        lat, lon
    );

#if DEBUG_GEO
    Serial.printf("[GEO] URL: %s\n", url);
#endif

    http.begin(url);
    http.setUserAgent("AmbiSense/1.0 (ESP32)");
    http.setTimeout(10000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int code = http.GET();

#if DEBUG_GEO
    Serial.printf("[GEO] HTTP code: %d\n", code);
#endif

    if (code == 200) {
        String payload = http.getString();

#if DEBUG_GEO
        Serial.println("[GEO] Response: " + payload);
#endif

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, payload);

        if (!err) {
            JsonObject addr = doc["address"];

            const char* city = addr["city"];
            if (!city) city = addr["town"];
            if (!city) city = addr["village"];
            if (!city) city = addr["state"];

            if (city) {
#if DEBUG_GEO
                Serial.printf("[GEO] Found city: %s\n", city);
#endif
                http.end();
                return String(city);
            }
        } else {
#if DEBUG_GEO
            Serial.printf("[GEO] JSON parse error: %s\n", err.c_str());
#endif
        }
    } else {
#if DEBUG_GEO
        Serial.printf("[GEO] Failed with code: %d\n", code);
#endif
    }

    http.end();
    return "Unknown";
}