/* ==================== Network.cpp ==================== */
#include "Network.h"

/* =============== INCLUDES =============== */
/* ============ PROJECT ============ */
#include "config/LocationConfig.h"

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

    /* --- System --- */
    pkt.type          = PACKET_TYPE_DATA;
    pkt.seq           = _seqCounter++;
    pkt.channel       = WiFi.channel();
    pkt.wifiConnected = isConnected() ? 1 : 0;

    /* --- RTC --- */
    if (_rtc) {
        // Get the real time from the chip and put it in the packet
        DateTime dt = _rtc->getTime();
        pkt.timestamp = (uint32_t)dt.unixtime();
    }

    /* --- Location --- */
    // If location not valid, request it (non-blocking)
    if (!_locationResolver.isValid()) {
        _locationResolver.getCityName(LOCATION_LAT, LOCATION_LON);
    }

    String city = _locationResolver.getCachedCity();
    if (_locationResolver.isValid() && city.length() > 0) {
        strlcpy(pkt.city, city.c_str(), sizeof(pkt.city));
        pkt.locationValid = 1;
    } else {
        strlcpy(pkt.city, "Unknown", sizeof(pkt.city));
        pkt.locationValid = 0;
    }

    /* --- Weather --- */
    pkt.weatherValid    = w.valid ? 1 : 0;
    pkt.weatherCode     = w.weatherCode;
    pkt.outsideTemp     = w.temp;
    pkt.apparentTemp    = w.apparentTemp;
    pkt.outsideHumi     = w.humidity;
    pkt.outsidePress    = w.pressure;
    pkt.windSpeed       = w.windSpeed;
    pkt.windDirection   = w.windDirection;
    strlcpy(pkt.sunrise, w.sunrise, sizeof(pkt.sunrise));
    strlcpy(pkt.sunset,  w.sunset,  sizeof(pkt.sunset));

    /* --- Room Metrics --- */
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
    , _wifiState(WifiState::IDLE)
    , _connectTimeoutMs(0)
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

    _locationResolver.begin();
    _weather.begin();

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

    Serial.println("[NET] ESP-NOW INIT");
    
    if (!loadConfig()) {
        Serial.println("[NET] Using default config...");
        strlcpy(_ssid, "YOUR_SSID_HERE", sizeof(_ssid));
        strlcpy(_password, "YOUR_PASSWORD_HERE", sizeof(_password));
        strlcpy(_ntpServer, "asia.pool.ntp.org", sizeof(_ntpServer));
        _hasConfig = true; // Set this so update() doesn't block!
    } else {
        connectWiFi();
    }
}

/* ========= update ========= */
void Network::update() {
    _now = millis();

    /* ------ LocationResolver ------ */
    _locationResolver.update();

    /* ------ Wi-Fi State Machine ------ */
    switch (_wifiState) {
        case WifiState::CONNECTING:
            if (WiFi.status() == WL_CONNECTED) {
                _wifiState = WifiState::CONNECTED;
                esp_wifi_set_ps(WIFI_PS_NONE);
                Serial.printf("\n[NET] Connected to IP: %s\n", WiFi.localIP().toString().c_str());
            } 
            else if (_now - _connectTimeoutMs > WIFI_CONNECT_TIMEOUT_MS) {
                Serial.println("\n[NET] Connection timed out");
                _wifiState = WifiState::WAIT_RETRY;
                _lastReconnectAttempt = _now;
            }
            break;

        case WifiState::CONNECTED:
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("[NET] Wi-Fi Lost");
                _wifiState = WifiState::WAIT_RETRY;
                _lastReconnectAttempt = _now;
            }
            break;

        case WifiState::WAIT_RETRY:
        case WifiState::IDLE:
            if (_hasConfig && (_now - _lastReconnectAttempt > WIFI_RETRY_INTERVAL_MS)) {
                connectWiFi();
            }
            break;
    }

    /* ------ RTC Maintenance ------ */
    if (_rtc) {
        _rtc->update(_now, isConnected(), _ntpServer);
    }

    /* ------ Periodic broadcast ------ */
    if (_now - _lastBroadcastMs >= BROADCAST_INTERVAL_MS) {
        _lastBroadcastMs = _now;
        
        WeatherData w = _weather.getData();
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

    Serial.printf("[NET] Connecting to %s...\n", _ssid);
    WiFi.begin(_ssid, _password);
    
    _wifiState = WifiState::CONNECTING;

    // If we are calling this from begin(), _now might be 0. 
    // To be safe, we can either use millis() here or ensure _now is updated.
    _now = millis(); 
    _connectTimeoutMs = _now;
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
