/* ==================== main.cpp ==================== */
/* AmbiSense Display — ESP32-2432S028 / CYD          */

/* =============== INCLUDES =============== */
/* ============ PROJECT ============ */
#include "DisplayManager.h"
#include "Network.h"
#include "UI.h"

/* ============ CORE ============ */
#include <Arduino.h>
#include <time.h>

/* =============== INTERNAL STATE =============== */
/* ============ STATIC VARS ============ */
static DisplayManager display;
static Network        network;
static UI             ui;

static WeatherData       g_weather   = {};
static volatile uint32_t g_timestamp = 0;
static volatile float    g_roomTemp  = -99.0f;
static volatile float    g_roomHumi  = -99.0f;
static volatile bool     g_roomValid = false;
static volatile bool     g_hubWifi   = false;
static volatile uint32_t g_lastPktMs = 0;   /* millis() of last DataPacket — for offline detection */

/* =============== INTERNAL HELPERS =============== */
/* ============ CALLBACKS ============ */
/* ------ onData ------ */
static void onData(const DataPacket& pkt) {
    g_timestamp = pkt.timestamp;

    g_roomValid = (pkt.roomValid != 0);
    g_roomTemp  = pkt.roomTemp;
    g_roomHumi  = pkt.roomHumi;
    g_hubWifi   = (pkt.wifiConnected != 0);

    g_weather.valid         = (pkt.weatherValid != 0);
    g_weather.temp          = pkt.outsideTemp;
    g_weather.feelsLike     = pkt.apparentTemp;
    g_weather.humidity      = pkt.outsideHumi;
    g_weather.pressure      = pkt.outsidePressure;
    g_weather.windSpeed     =  pkt.windSpeed;
    g_weather.windDirection = pkt.windDir;
    g_weather.weatherCode   = pkt.weatherCode;
    strncpy(g_weather.sunrise, pkt.sunrise, sizeof(g_weather.sunrise));
    strncpy(g_weather.sunset,  pkt.sunset,  sizeof(g_weather.sunset));
}

/* ------ onConfigSubmit ------ */
static void onConfigSubmit(const char* ssid, const char* password, const char* ntp) {
    network.saveConfig(ssid, password, ntp);
    network.sendConfig(ssid, password, ntp);
}

/* ------ onForceSync ------ */
static void onForceSync() {
    network.sendCmd(CMD_FORCE_NTP_SYNC);
}

/* ------ onStatusChanged ------ */
static void onStatusChanged(bool online) {
    if (!online) {
        g_weather.valid = false;
        g_roomValid     = false;
    }
}

/* =============== PUBLIC API =============== */
/* ============ LIFECYCLE ============ */
/* ========= setup ========= */
void setup() {
    Serial.begin(115200);
    Serial.println("[MAIN] AmbiSense Display booting...");

    /* --- Bangkok UTC+7, no DST --- */
    setenv("TZ", "ICT-7", 1);
    tzset();

    display.begin();

    network.begin();
    network.setOnDataReceived(onData);
    network.setOnStatusChanged(onStatusChanged);

    ui.setOnConfigSubmit(onConfigSubmit);
    ui.setOnForceSyncCmd(onForceSync);
    ui.begin();

    Serial.println("[MAIN] Boot complete.");
}

/* ========= loop ========= */
void loop() {
    display.update();
    network.update();

    static uint32_t lastUIUpdate = 0;
    if (millis() - lastUIUpdate >= 100) {
        lastUIUpdate = millis();
        ui.update(g_timestamp, g_roomTemp, g_roomHumi, g_roomValid,
                  g_weather, network.isOnline());
    }
}