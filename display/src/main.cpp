/* ==================== main.cpp ==================== */

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
static volatile uint32_t g_lastPktMs = 0;   /* millis() of last DataPacket — for offline detection */
static SemaphoreHandle_t g_pktMutex = nullptr;

/* =============== INTERNAL HELPERS =============== */
/* ============ CALLBACKS ============ */
/* ------ onData ------ */
static void onData(const DataPacket& pkt) {
    if (g_pktMutex && xSemaphoreTake(g_pktMutex, portMAX_DELAY)) {
        g_lastPkt = pkt;
        g_lastPktMs = millis();
        xSemaphoreGive(g_pktMutex);
    }
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

/* =============== PUBLIC API =============== */
/* ============ LIFECYCLE ============ */
/* ========= setup ========= */
void setup() {
    Serial.begin(115200);
    Serial.println("[MAIN] AmbiSense Display booting...");

    g_pktMutex = xSemaphoreCreateMutex();

    /* --- Bangkok UTC+7, no DST --- */
    setenv("TZ", "ICT-7", 1);
    tzset();

    display.begin();

    network.begin();
    network.setOnDataReceived(onData);
    // Remove onStatusChanged – UI will use network.isOnline() directly

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
        
        DataPacket localPkt;
        if (g_pktMutex && xSemaphoreTake(g_pktMutex, portMAX_DELAY)) {
            localPkt = g_lastPkt;
            xSemaphoreGive(g_pktMutex);
        }
        
        ui.update(localPkt, network.isOnline());
    }
}