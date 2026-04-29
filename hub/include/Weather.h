/* ========== Weather.h ========== */
#pragma once

/* ===== INCLUDES ===== */
/* --- PROJECT --- */
#include "config/LocationConfig.h"

/* --- THIRD-PARTY --- */
#include <ArduinoJson.h>

/* --- CORE --- */
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "freertos/semphr.h"

struct WeatherData {
    float   temp;
    float   apparentTemp;
    int     humidity;
    float   windSpeed;
    int     windDirection;
    int     pressure;
    int     weatherCode;
    char    sunrise[6];   // "HH:MM"
    char    sunset[6];    // "HH:MM"
    bool    valid;
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

/* ===== WEATHER CODE HELPERS ===== */
const char* getWeatherDesc(int code);
const char* getWeatherIcon(int code);
const char* getWindDir(int deg);
