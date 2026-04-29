/* ==================== Network.cpp ==================== */
#include "Network.h"

/* =============== INCLUDES =============== */
/* ============ CORE ============ */
#include <Arduino.h>
#include <sys/time.h>

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
    if (!mac || !data || len <= 0) return;
    uint8_t type = data[0];

    /* ========= DATA ========= */
    if (type == PACKET_TYPE_DATA && len == sizeof(DataPacket)) {
        DataPacket pkt;
        memcpy(&pkt, data, sizeof(pkt));
        
        /* --- CHANNEL AUTO-SYNC --- */
        // If the Hub is on a different channel than us, switch immediately
        uint8_t hubChan = pkt.channel;
        if (hubChan > 0 && hubChan <= 13 && hubChan != _lastKnownHubChan) {
            esp_wifi_set_channel(hubChan, WIFI_SECOND_CHAN_NONE);
            _lastKnownHubChan = hubChan;
            Serial.printf("[NET] LOCKED to Hub Channel: %d\n", hubChan);
            _seqInitialized = false; 
        }

        /* --- OFFLINE DETECTION & RECOVERY --- */
        _lastPacketMs = millis();
        if (!_isConnected) {
            _isConnected = true;
            uint32_t syncTime = millis() - _scanStartMs;
            Serial.printf("[NET] Connection Restored in %ums on Channel %d\n", syncTime, pkt.channel);
            if (_onStatus) _onStatus(true); // Tell UI we are ONLINE
        }

        /* --- CLOCK SYNC --- */
        if (pkt.timestamp > 1000000000UL) {
            struct timeval tv = { (time_t)pkt.timestamp, 0 };
            settimeofday(&tv, nullptr);
        }

        /* --- HUB MAC LEARNING --- */
        if (!_hasHubMac) {
            memcpy(_hubMac, mac, 6);
            _hasHubMac = true;
            esp_now_peer_info_t peer = {};
            memcpy(peer.peer_addr, _hubMac, 6);
            peer.channel = 0;
            peer.encrypt = false;
            if (!esp_now_is_peer_exist(_hubMac)) esp_now_add_peer(&peer);
            Serial.println("[NET] Hub MAC learned");
        }

        /* --- SEQUENCE TRACKING --- */
        if (_seqInitialized) {
            uint8_t diff = pkt.seq - _lastSeq;
            if (diff != 1) {
                _dropCount += (uint32_t)(diff - 1);
                #if DEBUG_NETWORK
                Serial.printf("[NET] Seq drop: last=%u now=%u dropped=%u\n", _lastSeq, pkt.seq, (uint8_t)(diff - 1));
                #endif
            }
        } else {
            _seqInitialized = true;
        }
        _lastSeq = pkt.seq;

        if (_onData) _onData(pkt);
        _sendAck(mac, pkt.seq);
    }
    /* ========= ACK ========= */
    else if (type == PACKET_TYPE_ACK && len == sizeof(AckPacket)) {
        AckPacket pkt;
        memcpy(&pkt, data, sizeof(pkt));
        if (_onAck) _onAck(pkt.ack_seq);
    }
    /* ========= UNKNOWN ========= */
    else {
        Serial.printf("[NET] Unknown type 0x%02X len=%d\n", type, len);
    }
}

/* ============ LOGIC ============ */
/* ========= TRANSMISSION ========= */
/* ------ _sendAck ------ */
void Network::_sendAck(const uint8_t* mac, uint8_t seq) {
    AckPacket pkt = {};
    pkt.type    = PACKET_TYPE_ACK;
    pkt.ack_seq = seq;
    esp_now_send(mac, (uint8_t*)&pkt, sizeof(pkt));
}

/* =============== PUBLIC API =============== */
/* ============ LIFECYCLE ============ */
/* ========= constructor ========= */
Network::Network()
    : _hasConfig(false)
    , _hasHubMac(false)
    , _lastKnownHubChan(0)
    , _lastPacketMs(0)
    , _isConnected(false)
    , _scanStartMs(0)
    , _lastHopMs(0)
    , _currentScanChan(13)
    , _txSeq(0)
    , _lastSeq(0)
    , _seqInitialized(false)
    , _dropCount(0)
    , _onData(nullptr)
    , _onAck(nullptr)
    , _onStatus(nullptr)
    , _lastReconnectAttempt(0)
{
    memset(_ssid,      0, sizeof(_ssid));
    memset(_password,  0, sizeof(_password));
    memset(_ntpServer, 0, sizeof(_ntpServer));
    memset(_hubMac,    0, sizeof(_hubMac));
    _instance = this;
}

/* ========= begin ========= */
void Network::begin() {
    /* ------ WiFi: STA mode, stay disconnected so ESP-NOW can use any channel ------ */
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    esp_wifi_set_ps(WIFI_PS_NONE);
    esp_wifi_set_channel(0, WIFI_SECOND_CHAN_NONE);
    Serial.println("[NET] Scanning on ALL Channels...");

    /* ------ ESP-NOW ------ */
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
    if (!esp_now_is_peer_exist(ESPNOW_BROADCAST)) esp_now_add_peer(&peer);

    loadConfig();
    Serial.println("[NET] ESP-NOW ready");
}

/* ========= update ========= */
void Network::update() {
    uint32_t now = millis();
    const uint32_t TIMEOUT_THRESHOLD = 15000; 

    if (_isConnected) {
        /* --- WATCHDOG FOR DISCONNECTION --- */
        if (now - _lastPacketMs > TIMEOUT_THRESHOLD) {
            _isConnected = false;
            _lastKnownHubChan = 0;
            _currentScanChan = 13; // Restart scan from channel 13
            _scanStartMs = now;
            Serial.println("[NET] Connection Lost. Starting Active Scan...");
            if (_onStatus) _onStatus(false);
        }
    } else {
        /* --- ACTIVE CHANNEL HOPPING --- */
        if (now - _lastHopMs > SCAN_HOP_INTERVAL_MS) {
            _lastHopMs = now;
            
            // Move to the next channel
            _currentScanChan++;
            if (_currentScanChan > 13) _currentScanChan = 1;

            esp_wifi_set_channel(_currentScanChan, WIFI_SECOND_CHAN_NONE);
            
            #if DEBUG_NETWORK
            Serial.printf("[NET] Scanning Channel: %d\n", _currentScanChan);
            #endif
        }
    }
}

/* ============ ACTIONS ============ */
/* ========= sendConfig ========= */
void Network::sendConfig(const char* ssid, const char* password, const char* ntp) {
    ConfigPacket pkt = {};
    pkt.type = PACKET_TYPE_CONFIG;
    strlcpy(pkt.ssid,     ssid,     sizeof(pkt.ssid));
    strlcpy(pkt.password, password, sizeof(pkt.password));
    strlcpy(pkt.ntp,      ntp,      sizeof(pkt.ntp));
    pkt.seq = _txSeq++;
    esp_now_send(ESPNOW_BROADCAST, (uint8_t*)&pkt, sizeof(pkt));
    Serial.printf("[NET] Config TX seq=%u\n", pkt.seq);
}

/* ========= sendCmd ========= */
void Network::sendCmd(uint8_t cmd) {
    CmdPacket pkt = {};
    pkt.type = PACKET_TYPE_CMD;
    pkt.cmd  = cmd;
    pkt.seq  = _txSeq++;
    esp_now_send(ESPNOW_BROADCAST, (uint8_t*)&pkt, sizeof(pkt));
    Serial.printf("[NET] CMD 0x%02X TX seq=%u\n", cmd, pkt.seq);
}

/* ============ STORAGE ============ */
/* ========= loadConfig ========= */
bool Network::loadConfig() {
    _prefs.begin("ambisense", true);
    String ssid = _prefs.getString("ssid", "");
    if (ssid.length() == 0) { _prefs.end(); return false; }
    strlcpy(_ssid,      ssid.c_str(),                                    sizeof(_ssid));
    strlcpy(_password,  _prefs.getString("pass", "").c_str(),            sizeof(_password));
    strlcpy(_ntpServer, _prefs.getString("ntp", "asia.pool.ntp.org").c_str(), sizeof(_ntpServer));
    _prefs.end();
    _hasConfig = true;
    #if DEBUG_NETWORK
    Serial.printf("[NET] Config loaded: SSID: %s\n", _ssid);
    #endif
    return true;
}

/* ========= saveConfig ========= */
void Network::saveConfig(const char* ssid, const char* password, const char* ntp) {
    strlcpy(_ssid,      ssid,     sizeof(_ssid));
    strlcpy(_password,  password, sizeof(_password));
    strlcpy(_ntpServer, ntp,      sizeof(_ntpServer));
    _prefs.begin("ambisense", false);
    _prefs.putString("ssid", _ssid);
    _prefs.putString("pass", _password);
    _prefs.putString("ntp",  _ntpServer);
    _prefs.end();
    _hasConfig = true;
    Serial.printf("[NET] Config saved: SSID=%s\n", _ssid);
}
