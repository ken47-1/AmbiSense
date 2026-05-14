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
    void   begin();
    void   update();  // Call this from Network::update()
    String getCityName(float lat, float lon);  // Will wait for WiFi internally
    String getCachedCity() const { return _cachedCity; }
    bool   isValid() const { return _valid; }
    
private:
    String   _fetchFromAPI(float lat, float lon);
    
    String   _cachedCity;
    bool     _valid;
    uint32_t _lastAttemptMs;
    uint8_t  _retryCount;
    float    _pendingLat;
    float    _pendingLon;
    bool     _fetchPending;
};