/* ==================== LocationResolver.h ==================== */
#pragma once

/* =============== INCLUDES =============== */
/* ============ THIRD-PARTY ============ */
#include <ArduinoJson.h>
#include <HTTPClient.h>

/* ============ CORE ============ */
#include <Arduino.h>

/* =============== API =============== */
class LocationResolver {
public:
    LocationResolver();
    void begin();
    String getCityName(float lat, float lon);
    String getCachedCity() const { return _cachedCity; }
    bool isValid() const { return _valid; }
    
private:
    String _fetchFromAPI(float lat, float lon);
    String _cachedCity;
    bool _valid;
};