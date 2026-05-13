/* ==================== LocationResolver.cpp ==================== */
#include "LocationResolver.h"

/* =============== INCLUDES =============== */
/* ============ PROJECT ============ */
#include "config/Config.h"
#include "config/LocationConfig.h"

/* ============ CORE ============ */
#include <Arduino.h>
#include <WiFi.h>

/* =============== PUBLIC API =============== */
/* ============ LIFECYCLE ============ */
LocationResolver::LocationResolver() 
    : _valid(false)
    , _lastAttemptMs(0)
    , _retryCount(0)
    , _pendingLat(0)
    , _pendingLon(0)
    , _fetchPending(false)
{}

void LocationResolver::begin() {
    _valid = false;
    _cachedCity = "Unknown";
    _retryCount = 0;
    _fetchPending = false;
    Serial.println("[GEO] LocationResolver INIT");
}

void LocationResolver::update() {
    // Don't try if already valid or no pending fetch
    if (_valid || !_fetchPending) return;
    
    // Don't try if WiFi not connected
    if (WiFi.status() != WL_CONNECTED) return;
    
    // Rate limiting
    uint32_t now = millis();
    if (now - _lastAttemptMs < LOCATION_RETRY_INTERVAL_MS) return;
    _lastAttemptMs = now;
    
    // Attempt fetch
    String result = _fetchFromAPI(_pendingLat, _pendingLon);
    
    if (result != "Unknown") {
        _cachedCity = result;
        _valid = true;
        _fetchPending = false;
        _retryCount = 0;
        Serial.printf("[GEO] Location resolved: %s\n", _cachedCity.c_str());
    } else {
        _retryCount++;
        if (_retryCount >= LOCATION_MAX_RETRIES) {
            Serial.println("[GEO] Max retries reached, giving up until next reboot");
            _fetchPending = false;  // Stop trying
        } else {
            Serial.printf("[GEO] Retry %d/%d in %lu ms\n", _retryCount, LOCATION_MAX_RETRIES, LOCATION_RETRY_INTERVAL_MS);
        }
    }
}

/* ============ ACTIONS ============ */
String LocationResolver::getCityName(float lat, float lon) {
    // If already cached, return immediately
    if (_valid && _cachedCity.length() > 0) {
        return _cachedCity;
    }
    
    // Store pending request for update() to handle
    _pendingLat = lat;
    _pendingLon = lon;
    _fetchPending = true;
    _retryCount = 0;
    _valid = false;
    
    // Try once immediately if WiFi is ready
    if (WiFi.status() == WL_CONNECTED) {
        update();
    }
    
    // Return whatever we have (likely "Unknown" for now)
    return _cachedCity;
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

    http.begin(url);
    http.setUserAgent("AmbiSense/1.0 (ESP32)");
    http.setTimeout(10000);

    int code = http.GET();

    if (code != 200) {
        http.end();
        return "Unknown";
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);

    if (err) {
        return "Unknown";
    }

    JsonObject addr = doc["address"];
    const char* city = addr["city"];
    if (!city) city = addr["town"];
    if (!city) city = addr["village"];
    if (!city) city = addr["state"];
    if (!city) city = addr["country"];

    return city ? String(city) : "Unknown";
}