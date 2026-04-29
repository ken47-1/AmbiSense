/* ========== Weather.cpp ========== */
#include "Weather.h"

/* ===== INCLUDES ===== */
/* --- PROJECT --- */
#include "config\Config.h"

/* Constructor */
Weather::Weather() {
    _mutex = xSemaphoreCreateMutex();

    memset(&_data, 0, sizeof(_data));
    _data.valid = false;

    strlcpy(_data.sunrise, "--:--", sizeof(_data.sunrise));
    strlcpy(_data.sunset, "--:--", sizeof(_data.sunset));
}

/* ===== HELPERS ===== */
void Weather::task_entry(void* arg) {
    Weather* self = static_cast<Weather*>(arg);

    bool wifiWasConnected = false;

    while (true) {
        bool wifiConnected = (WiFi.status() == WL_CONNECTED);

        if (wifiConnected) {
            if (!wifiWasConnected) {
                wifiWasConnected = true;
                self->_fetch(); // immediate fetch on connect
            } else {
                vTaskDelay(pdMS_TO_TICKS(WEATHER_INTERVAL_MS));
                self->_fetch(); // periodic refresh
            }
        } else {
            wifiWasConnected = false;
            vTaskDelay(pdMS_TO_TICKS(5000)); // check again in 5s
        }
    }
}

/* ===== PUBLIC API ===== */
void Weather::begin() {
    xTaskCreatePinnedToCore(
        task_entry,
        "weather_task",
        8192,
        this,
        1,
        nullptr,
        0
    );
}

void Weather::forceFetch() {
    if (WiFi.status() == WL_CONNECTED) {
        _fetch();
    }
}

WeatherData Weather::getData() {
    WeatherData copy;

    if (xSemaphoreTake(_mutex, portMAX_DELAY)) {
        copy = _data;
        xSemaphoreGive(_mutex);
    }

    return copy;
}

void Weather::_fetch() {
    HTTPClient http;

    char url[256];
    snprintf(url, sizeof(url),
        "https://api.open-meteo.com/v1/forecast"
        "?latitude=%.6f&longitude=%.6f"
        "&current=temperature_2m,apparent_temperature,relative_humidity_2m,"
        "weather_code,wind_speed_10m,wind_direction_10m,pressure_msl"
        "&daily=sunrise,sunset&timezone=auto",
        LOCATION_LAT, LOCATION_LON
    );

    http.setTimeout(8000);
    
    int code = -1;
    for (int attempt = 0; attempt < WEATHER_MAX_RETRIES; attempt++) {
        http.begin(url);
        code = http.GET();
        Serial.printf("[WEATHER] HTTP code: %d (attempt %d/%d)\n", code, attempt + 1, WEATHER_MAX_RETRIES);
        
        if (code == 200) break;
        http.end();
        
        if (code >= 400 && code < 500) break;
        
        if (attempt < WEATHER_MAX_RETRIES - 1) {
            uint32_t delay_ms = WEATHER_RETRY_DELAY_MS * (attempt + 1);
            Serial.printf("[WEATHER] Retrying in %lu ms...\n", delay_ms);
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }
    }

    if (code == 200) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, payload);

        if (!err) {
            if (xSemaphoreTake(_mutex, portMAX_DELAY)) {
                _data.temp          = doc["current"]["temperature_2m"];
                _data.apparentTemp  = doc["current"]["apparent_temperature"];
                _data.humidity      = doc["current"]["relative_humidity_2m"];
                _data.weatherCode   = doc["current"]["weather_code"];
                _data.windSpeed     = doc["current"]["wind_speed_10m"];
                _data.windDirection = doc["current"]["wind_direction_10m"];
                _data.pressure      = doc["current"]["pressure_msl"];

                if (doc["daily"]["sunrise"]) {
                    const char* sunriseRaw = doc["daily"]["sunrise"][0];
                    if (sunriseRaw) {
                        const char* tPtr = strchr(sunriseRaw, 'T');
                        if (tPtr) strncpy(_data.sunrise, tPtr + 1, 5);
                    }
                }
                
                if (doc["daily"]["sunset"]) {
                    const char* sunsetRaw = doc["daily"]["sunset"][0];
                    if (sunsetRaw) {
                        const char* tPtr = strchr(sunsetRaw, 'T');
                        if (tPtr) strncpy(_data.sunset, tPtr + 1, 5);
                    }
                }

                _data.sunrise[5] = '\0';
                _data.sunset[5] = '\0';
                _data.valid = true;
                
                xSemaphoreGive(_mutex);
                Serial.println("[WEATHER] Data fetched successfully");
            }
        } else {
            Serial.printf("[WEATHER] JSON error: %s\n", err.c_str());
            _setErrorState();
        }
    } else {
        Serial.printf("[WEATHER] Failed after %d attempts. Last code: %d\n", WEATHER_MAX_RETRIES, code);
        _setErrorState();
    }
    http.end();
}

void Weather::_setErrorState() {
    if (xSemaphoreTake(_mutex, portMAX_DELAY)) {
        _data.valid = false;
        xSemaphoreGive(_mutex);
    }
}
