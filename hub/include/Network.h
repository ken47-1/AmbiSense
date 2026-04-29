/* ==================== Network.h ==================== */
#pragma once

/* =============== INCLUDES =============== */
/* ============ PROJECT ============ */
#include "config/Config.h"
#include "Sensors.h"
#include "RTCManager.h"
#include "Weather.h"

/* ============ THIRD-PARTY ============ */
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>

/* ============ CORE ============ */
#include <Arduino.h>

/* =============== TYPES =============== */
/* ============ CALLBACKS ============ */
using OnConfigReceived = void(*)(const ConfigPacket&);
using OnCmdReceived    = void(*)(const CmdPacket&);

/* =============== API =============== */
class Network {
/* ============ PUBLIC ============ */
public:
    Network();

    /* ========= LIFECYCLE ========= */
    void begin();
    void update();

    /* ========= ACTIONS ========= */
    void broadcastData(const DataPacket& pkt);
    void sendAck(const uint8_t* mac, uint8_t ackSeq);
    void connectWiFi();

    /* ========= STORAGE ========= */
    bool loadConfig();
    void saveConfig(const ConfigPacket& pkt);

    /* ========= CALLBACKS ========= */
    void setOnConfigReceived(OnConfigReceived cb) { _onConfig = cb; }
    void setOnCmdReceived(OnCmdReceived       cb) { _onCmd    = cb; }

    /* ========= MODULE WIRING ========= */
    void attachModules(Sensors* sensors, RTCManager* rtc);

    /* ========= GETTERS ========= */
    bool        isConnected()  const { return WiFi.status() == WL_CONNECTED; }
    bool        hasConfig()    const { return _hasConfig; }
    const char* getSSID()      const { return _ssid; }
    const char* getPassword()  const { return _password; }
    const char* getNTPServer() const { return _ntpServer; }

    /* ========= ESP-NOW INTERNALS ========= */
    static void     _onDataRecv(const uint8_t* mac, const uint8_t* data, int len);
    static Network* _instance;

/* ============ PRIVATE ============ */
private:
    /* ========= HANDLERS ========= */
    void _handleReceived(const uint8_t* mac, const uint8_t* data, int len);
    void _buildDataPacket(DataPacket& pkt, const WeatherData& w);

    /* ========= STATE ========= */
    /* ------ Module references ------ */
    Sensors*    _sensors;
    RTCManager* _rtc;
    Weather     _weather;

    /* ------ Flash ------ */
    Preferences _prefs;

    /* ------ Config ------ */
    char _ssid[33];
    char _password[64];
    char _ntpServer[64];
    bool _hasConfig;

    /* ------ Broadcast ------ */
    uint8_t  _seqCounter;
    uint32_t _lastBroadcastMs;
    uint32_t _lastReconnectAttempt;

    /* ------ Callbacks ------ */
    OnConfigReceived _onConfig;
    OnCmdReceived    _onCmd;
};
