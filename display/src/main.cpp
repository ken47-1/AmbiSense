/* ==================== main.cpp ==================== */
/* !!! AmbiSense Display !!! */

/* =============== INCLUDES =============== */

/* ============ CONFIG ============ */
#include "config/DisplayConfig.h"

/* ============ PROJECT ============ */
#include "log/log.h"
#include "network/network.h"
#include "display/display_manager.h"
#include "display/ui.h"

/* ============ CORE ============ */
#include <Arduino.h>
#include <time.h>

/* =============== INTERNAL STATE =============== */
/* ============ STATIC VARS ============ */
static DataPacket        g_lastPkt = {};
static SemaphoreHandle_t g_pktMutex = nullptr;

/* ============ SINGLETONS ============ */
static AmbiSense::Display::DisplayManager display;
static AmbiSense::Display::Network        network;
static AmbiSense::Display::UI             ui;

/* =============== INTERNAL HELPERS =============== */
/* ============ CALLBACKS ============ */
static void onData(const DataPacket& pkt) {
    if (g_pktMutex && xSemaphoreTake(g_pktMutex, portMAX_DELAY)) {
        g_lastPkt = pkt;
        xSemaphoreGive(g_pktMutex);
    }
}

static void onConfigSubmit(const char* ssid, const char* password, const char* ntp) {
    network.saveConfig(ssid, password, ntp);
    network.sendConfig(ssid, password, ntp);
}

static void onForceSync() {
    network.sendCmd(CMD_FORCE_NTP_SYNC);
}

/* =============== PUBLIC API =============== */
/* ============ LIFECYCLE ============ */
void setup() {
    Serial.begin(115200);
    Log::init();    
    LOG_I(Log::Ch::CH_SYS, "AmbiSense Display booting...");

    g_pktMutex = xSemaphoreCreateMutex();

    setenv("TZ", "ICT-7", 1);
    tzset();

    display.begin();

    network.begin();
    network.setOnDataReceived(onData);

    ui.setOnConfigSubmit(onConfigSubmit);
    ui.setOnForceSyncCmd(onForceSync);
    ui.begin();

    LOG_I(Log::Ch::CH_SYS, "Boot complete.");
}

void loop() {
    display.update();
    network.update();

    uint32_t now = millis();
    static uint32_t lastUIUpdate = 0;
    if (now - lastUIUpdate >= 100) {
        lastUIUpdate = now;
        
        DataPacket localPkt = {};
        if (g_pktMutex && xSemaphoreTake(g_pktMutex, portMAX_DELAY)) {
            localPkt = g_lastPkt;
            xSemaphoreGive(g_pktMutex);
        }
        
        ui.update(localPkt, network.isOnline());
    }
}