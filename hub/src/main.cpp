/* ==================== main.cpp ==================== */
/* AmbiSense Hub                                      */

/* =============== INCLUDES =============== */
/* ============ PROJECT ============ */
#include "config/Config.h"
#include "Sensors.h"
#include "RTCManager.h"
#include "Weather.h"
#include "Network.h"

/* ============ CORE ============ */
#include <Arduino.h>

/* =============== INTERNAL STATE =============== */
/* ============ STATIC VARS ============ */
static Sensors    sensors;
static RTCManager rtc;
static Network    network;

/* =============== INTERNAL HELPERS =============== */
/* ============ CALLBACKS ============ */
/* ------ onConfigReceived ------ */
static void onConfigReceived(const ConfigPacket& pkt) {
    /* Leaf: Network::saveConfig already reconnects if credentials changed */
    Serial.println("[MAIN] Config received from Display");
}

/* ------ onCmdReceived ------ */
static void onCmdReceived(const CmdPacket& pkt) {
    if (pkt.cmd == CMD_FORCE_NTP_SYNC) {
        Serial.println("[MAIN] Force NTP sync requested");
        rtc.forceSync();
    }
}

/* =============== PUBLIC API =============== */
/* ============ LIFECYCLE ============ */
/* ========= setup ========= */
void setup() {
    Serial.begin(115200);
    Serial.println("\n[MAIN] AmbiSense Hub booting...");

    sensors.begin();
    rtc.begin();

    network.attachModules(&sensors, &rtc);
    network.setOnConfigReceived(onConfigReceived);
    network.setOnCmdReceived(onCmdReceived);
    network.begin();

    Serial.println("[MAIN] Boot complete.");
}

/* ========= loop ========= */
void loop() {
    sensors.update();
    rtc.update(network.isConnected(), network.getNTPServer());
    network.update();
}
