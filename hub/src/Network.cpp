/* ==================== Network.cpp ==================== */
#include "Network.h"

/* =============== INCLUDES =============== */
/* ============ CORE ============ */
#include <Arduino.h>

/* =============== INTERNAL STATE =============== */
/* ============ SINGLETONS ============ */
Network* Network::_instance = nullptr;

/* =============== INTERNAL HELPERS =============== */
/* ============ CALLBACKS ============ */
/* ========= ESP-NOW ========= */
/* ------ _onDataRecv ------ */
void Network::_onDataRecv(const uint8_t* mac, const uint8_t* data, int len) {
    if (_instance) _instance->_handleReceived(mac, data, len);
}

/* ------ _handleReceived ------ */
void Network::_handleReceived(const uint8_t* mac, const uint8_t* data, int len) {
    if (len < 1) return;
    uint8_t type = data[0];

    /* ========= CONFIG ========= */
    if (type == PACKET_TYPE_CONFIG && len == sizeof(ConfigPacket)) {
        ConfigPacket pkt;
        memcpy(&pkt, data, sizeof(pkt));
        Serial.printf("[NET] Config received: SSID: %s\n", pkt.ssid);
        saveConfig(pkt);
        sendAck(mac, pkt.seq);
        if (_onConfig) _onConfig(pkt);
    }
    /* ========= CMD ========= */
    else if (type == PACKET_TYPE_CMD && len == sizeof(CmdPacket)) {
        CmdPacket pkt;
        memcpy(&pkt, data, sizeof(pkt));
        if (pkt.cmd == CMD_FORCE_NTP_SYNC) {
            Serial.println("[NET] Force NTP sync command received");
            configTime(0, 0, _ntpServer);
        }
        sendAck(mac, pkt.seq);
        if (_onCmd) _onCmd(pkt);
    }
    /* ========= ACK ========= */
    else if (type == PACKET_TYPE_ACK && len == sizeof(AckPacket)) {
        AckPacket pkt;
        memcpy(&pkt, data, sizeof(pkt));
        #if DEBUG_NETWORK
        Serial.printf("[NET] ACK from display seq=%u\n", pkt.ack_seq);
        #endif
    }
    /* ========= UNKNOWN ========= */
    else {
        Serial.printf("[NET] Unknown type 0x%02X len=%d\n", type, len);
    }
}

/* ============ LOGIC ============ */
/* ========= PACKET BUILD ========= */
/* ------ _buildDataPacket ------ */
void Network::_buildDataPacket(DataPacket& pkt, const WeatherData& w) {
    pkt      = {};
    pkt.type = PACKET_TYPE_DATA;

    /* --- System --- */
    if (_rtc) {
        DateTime now  = _rtc->getTime();
        time_t local_now;
        time(&local_now); 
        pkt.timestamp = (uint32_t)local_now;
    }
    pkt.seq           = _seqCounter++;
    pkt.channel       = WiFi.channel();
    pkt.wifiConnected = isConnected() ? 1 : 0;

    /* --- Weather --- */
    pkt.weatherValid    = w.valid ? 1 : 0;
    pkt.weatherCode     = w.weatherCode;
    pkt.outsideTemp     = w.temp;
    pkt.apparentTemp    = w.apparentTemp;
    pkt.outsideHumi     = w.humidity;
    pkt.outsidePressure = w.pressure;
    pkt.windSpeed       = w.windSpeed;
    pkt.windDir         = w.windDirection;
    strlcpy(pkt.sunrise, w.sunrise, sizeof(pkt.sunrise));
    strlcpy(pkt.sunset,  w.sunset,  sizeof(pkt.sunset));

    /* --- Room sensors --- */
    pkt.roomValid = (_sensors && _sensors->isValid()) ? 1 : 0;
    if (_sensors && _sensors->isValid()) {
        pkt.roomTemp = _sensors->getTemp();
        pkt.roomHumi = _sensors->getHumidity();
    } else {
        pkt.roomTemp = -99.0f;
        pkt.roomHumi = -99.0f;
    }
}

/* =============== PUBLIC API =============== */
/* ============ LIFECYCLE ============ */
/* ========= constructor ========= */
Network::Network()
    : _hasConfig(false)
    , _seqCounter(0)
    , _lastBroadcastMs(0)
    , _lastReconnectAttempt(0)
    , _onConfig(nullptr)
    , _onCmd(nullptr)
    , _sensors(nullptr)
    , _rtc(nullptr)
{
    memset(_ssid,      0, sizeof(_ssid));
    memset(_password,  0, sizeof(_password));
    memset(_ntpServer, 0, sizeof(_ntpServer));
    _instance = this;
}

/* ========= attachModules ========= */
void Network::attachModules(Sensors* sensors, RTCManager* rtc) {
    _sensors = sensors;
    _rtc     = rtc;
}

/* ========= begin ========= */
void Network::begin() {
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        Serial.println("[NET] ESP-NOW init failed");
        return;
    }
    esp_now_register_recv_cb(_onDataRecv);

    /* --- Broadcast peer --- */
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, ESPNOW_BROADCAST, 6);
    peer.channel = 0;
    peer.encrypt = false;
    esp_now_add_peer(&peer);

    Serial.println("[NET] ESP-NOW ready");
    
    if (!loadConfig()) {
        Serial.println("[NET] Using default config...");
        strlcpy(_ssid, "YOUR_SSID_HERE", sizeof(_ssid));
        strlcpy(_password, "YOUR_PASSWORD_HERE", sizeof(_password));
        strlcpy(_ntpServer, "asia.pool.ntp.org", sizeof(_ntpServer));
        _hasConfig = true; // Set this so update() doesn't block!
    } else {
        connectWiFi();
    }
    
    _weather.begin();
}

/* ========= update ========= */
void Network::update() {
    /* ------ Maintain Cloud Link (Weather & NTP updates) ------ */
    // If we have credentials but lost Wi-Fi, retry every 30 seconds
    if (_hasConfig && !isConnected() && (millis() - _lastReconnectAttempt > 30000)) {
        connectWiFi();
        _lastReconnectAttempt = millis();
    }

    /* ------ Periodic broadcast (Runs even if WiFi is down) ------ */
    if (millis() - _lastBroadcastMs >= BROADCAST_INTERVAL_MS) {
        _lastBroadcastMs = millis();
        
        WeatherData w = _weather.getData(); // Might return empty if no WiFi, that's fine
        DataPacket  pkt;
        
        _buildDataPacket(pkt, w); 
        broadcastData(pkt);
        
        #if DEBUG_NETWORK
        Serial.println("[NET] Periodic broadcast sent");
        #endif
    }
}

/* ============ ACTIONS ============ */
/* ========= connectWiFi ========= */
void Network::connectWiFi() {
    if (!_hasConfig) return;
    if (WiFi.status() == WL_CONNECTED) WiFi.disconnect();

    Serial.printf("[NET] Connecting to %s...\n", _ssid);
    WiFi.begin(_ssid, _password);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 12000) {
        delay(500);
        Serial.print(".");
    }

    if (isConnected()) {
        esp_wifi_set_ps(WIFI_PS_NONE);
        Serial.printf("\n[NET] Connected. IP=%s Channel=%d\n",
                      WiFi.localIP().toString().c_str(), WiFi.channel());
        /* --- Start NTP; RTCManager polls time() to detect completion --- */
        configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, _ntpServer);
        Serial.printf("[NET] NTP started: %s\n", _ntpServer);
    } else {
        Serial.println("\n[NET] Connection failed");
    }
}

/* ========= broadcastData ========= */
void Network::broadcastData(const DataPacket& pkt) {
    esp_now_send(ESPNOW_BROADCAST, (const uint8_t*)&pkt, sizeof(DataPacket));
}

/* ========= sendAck ========= */
void Network::sendAck(const uint8_t* mac, uint8_t ackSeq) {
    AckPacket ack = {};
    ack.type    = PACKET_TYPE_ACK;
    ack.ack_seq = ackSeq;
    esp_now_send(mac, (uint8_t*)&ack, sizeof(AckPacket));
}

/* ============ STORAGE ============ */
/* ========= loadConfig ========= */
bool Network::loadConfig() {
    _prefs.begin("ambisense", true);
    String ssid = _prefs.getString("ssid", "");
    if (ssid.length() == 0) { _prefs.end(); return false; }
    strlcpy(_ssid,      ssid.c_str(),                                    sizeof(_ssid));
    strlcpy(_password,  _prefs.getString("pass", "").c_str(),            sizeof(_password));
    strlcpy(_ntpServer, _prefs.getString("ntp", "pool.ntp.org").c_str(), sizeof(_ntpServer));
    _prefs.end();
    _hasConfig = true;
    Serial.printf("[NET] Config loaded: SSID: %s NTP: %s\n", _ssid, _ntpServer);
    return true;
}

/* ========= saveConfig ========= */
void Network::saveConfig(const ConfigPacket& pkt) {
    bool changed = (strcmp(_ssid, pkt.ssid) != 0 || strcmp(_password, pkt.password) != 0);

    strlcpy(_ssid,      pkt.ssid,     sizeof(_ssid));
    strlcpy(_password,  pkt.password, sizeof(_password));
    strlcpy(_ntpServer, pkt.ntp,      sizeof(_ntpServer));
    _hasConfig = true;

    _prefs.begin("ambisense", false);
    _prefs.putString("ssid", _ssid);
    _prefs.putString("pass", _password);
    _prefs.putString("ntp",  _ntpServer);
    _prefs.end();

    Serial.printf("[NET] Config saved: SSID: %s\n", _ssid);
    if (changed) connectWiFi();
}
