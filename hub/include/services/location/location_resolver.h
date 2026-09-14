/* ==================== location_resolver.h ==================== */
#pragma once

/* =============== INCLUDES =============== */
/* ============ THIRD-PARTY ============ */
#include <ArduinoJson.h>
#include <HTTPClient.h>

/* ============ CORE ============ */
#include <Arduino.h>

namespace AmbiSense::Hub {

/* =============== API =============== */
class LocationResolver {
public:
    LocationResolver();
    void   begin();
    void   update();  // Call this from Network::update()
    String getCityName(float lat, float lon);  // Queues the request; update() fetches.
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

} // namespace AmbiSense::Hub