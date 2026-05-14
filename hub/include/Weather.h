/* ==================== Weather.h ==================== */
#pragma once

/* =============== INCLUDES =============== */
/* ============ THIRD-PARTY ============ */
#include <ArduinoJson.h>
#include <HTTPClient.h>

/* ============ CORE ============ */
#include <Arduino.h>
#include <WiFi.h>
#include "freertos/semphr.h"

/* =============== TYPES =============== */
/* ============ STRUCTS ============ */
struct WeatherData {
    bool    valid;
    uint8_t weatherCode;
    float   temp;
    float   apparentTemp;
    int     humidity;
    int     pressure;
    float   windSpeed;
    int     windDirection;
    char    sunrise[8];
    char    sunset[8];
};

/* =============== API =============== */
class Weather {
public:
    Weather();
    void begin();
    void forceFetch();
    WeatherData getData();

private:
    void _fetch();
    void _setErrorState();
    static void task_entry(void* arg);

    WeatherData _data;
    SemaphoreHandle_t _mutex;
};