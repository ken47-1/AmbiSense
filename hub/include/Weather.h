/* ========== Weather.h ========== */
#pragma once

/* --- THIRD-PARTY --- */
#include <ArduinoJson.h>

/* --- CORE --- */
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "freertos/semphr.h"

struct WeatherData {
    bool    valid;
    
    uint8_t weatherCode;
    float   temp;
    
    float   apparentTemp;
    int     humidity;
    
    int     pressure;

    float   windSpeed;
    int     windDirection;
    char    sunrise[8];   // "HH:MM"
    char    sunset[8];    // "HH:MM"
    
};


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
