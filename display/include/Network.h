/* ==================== Network.h ==================== */
#pragma once

/* =============== INCLUDES =============== */
/* ============ PROJECT ============ */
#include "config/Config.h"

/* ============ THIRD-PARTY ============ */
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>

/* ============ CORE ============ */
#include <Arduino.h>

/* =============== TYPES =============== */
/* ============ CALLBACKS ============ */
using OnDataReceived  = void(*)(const DataPacket&);
using OnAckReceived   = void(*)(uint8_t);
using OnStatusChanged = void(*)(bool);

/* =============== API =============== */
class Network {
/* ============ PUBLIC ============ */
public:
    Network();

    /* ========= LIFECYCLE ========= */
    void begin();
    void update();

    /* ========= TX ========= */
    void sendConfig(const char* ssid, const char* password, const char* ntp);
    void sendCmd(uint8_t cmd);

    /* ========= STORAGE ========= */
    bool loadConfig();
    void saveConfig(const char* ssid, const char* password, const char* ntp);

    /* ========= CALLBACKS ========= */
    void setOnDataReceived(OnDataReceived cb)   { _onData = cb; }
    void setOnAckReceived(OnAckReceived   cb)   { _onAck  = cb; }
    void setOnStatusChanged(OnStatusChanged cb) { _onStatus = cb; }

    /* ========= GETTERS ========= */
    bool        isOnline()        const { return _isConnected; }
    bool        hasHubMac()       const { return _hasHubMac; }
    bool        hasConfig()       const { return _hasConfig; }
    uint32_t    getLastPacketMs() const { return _lastPacketMs; }
    const char* getSSID()         const { return _ssid; }
    const char* getPassword()     const { return _password; }

    /* ========= ESP-NOW INTERNALS ========= */
    static void     _onDataRecv(const uint8_t* mac, const uint8_t* data, int len);
    static Network* _instance;

/* ============ PRIVATE ============ */
private:
    /* ========= HANDLERS ========= */
    void _handleReceived(const uint8_t* mac, const uint8_t* data, int len);
    void _sendAck(const uint8_t* mac, uint8_t seq);

    /* ========= STATE ========= */
    /* ------ Flash ------ */
    Preferences _prefs;

    /* ------ Config ------ */
    char _ssid[33];
    char _password[64];
    char _ntpServer[64];
    bool _hasConfig;

    /* ------ Hub / Radio ------ */
    uint8_t  _hubMac[6];
    bool     _hasHubMac;
    uint8_t  _lastKnownHubChan;
    uint32_t _lastPacketMs;     /* millis() when last DataPacket arrived */
    bool     _isConnected;
    uint32_t _scanStartMs;      /* Track when we started looking for the Hub */
    uint32_t _lastHopMs;        /* Track time between channel jumps */
    uint8_t  _currentScanChan;  /* Current channel being checked (1-13) */

    /* ------ Sequence Tracking ------ */
    uint8_t  _txSeq;
    uint8_t  _lastSeq;
    bool     _seqInitialized;
    uint32_t _dropCount;

    /* ------ Callbacks ------ */
    OnDataReceived  _onData;
    OnAckReceived   _onAck;
    OnStatusChanged _onStatus;

    uint32_t _lastReconnectAttempt;
};
