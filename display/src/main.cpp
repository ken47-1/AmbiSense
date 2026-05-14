/* ==================== main.cpp ==================== */
/* !!! AmbiSense Display !!! */

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

static DataPacket        g_lastPkt = {};
static SemaphoreHandle_t g_pktMutex = nullptr;

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
    Serial.println("[MAIN] AmbiSense Display booting...");

    g_pktMutex = xSemaphoreCreateMutex();

    setenv("TZ", "ICT-7", 1);
    tzset();

    display.begin();

    network.begin();
    network.setOnDataReceived(onData);

    ui.setOnConfigSubmit(onConfigSubmit);
    ui.setOnForceSyncCmd(onForceSync);
    ui.begin();

    Serial.println("[MAIN] Boot complete.");
}

void loop() {
    display.update();
    network.update();

    static uint32_t lastUIUpdate = 0;
    if (millis() - lastUIUpdate >= 100) {
        lastUIUpdate = millis();
        
        DataPacket localPkt;
        if (g_pktMutex && xSemaphoreTake(g_pktMutex, portMAX_DELAY)) {
            localPkt = g_lastPkt;
            xSemaphoreGive(g_pktMutex);
        }
        
        ui.update(localPkt, network.isOnline());
    }
}