/* ==================== network.cpp ==================== */
#include "network/network.h"

/* =============== INCLUDES =============== */

/* ============ CONFIG ============ */
#include "config/PacketProtocol.h"
#include "config/DisplayConfig.h"
#include "config/DebugConfig.h"

/* ============ CORE ============ */
#include <sys/time.h>

namespace AmbiSense::Display {

/* =============== INTERNAL STATE =============== */
/* ============ SINGLETONS ============ */
/* File-scope access point for the ESP-NOW C-style receive callback.
   esp_now_register_recv_cb requires a free function pointer, so the
   static trampoline `_onDataRecv` needs a way back into this instance. */
Network* Network::_instance = nullptr;

/* =============== INTERNAL HELPERS =============== */
/* ============ CALLBACKS ============ */
/* ========= ESP-NOW ========= */
void Network::_onDataRecv(const uint8_t* mac, const uint8_t* data, int len) {
    if (_instance) _instance->_handleReceived(mac, data, len);
}

void Network::_handleReceived(const uint8_t* mac, const uint8_t* data, int len) {
    if (!mac || !data || len <= 0) return;
    uint8_t type = data[0];

    /* ========= DATA ========= */
    if (type == PACKET_TYPE_DATA && len == sizeof(DataPacket)) {
        DataPacket pkt;
        memcpy(&pkt, data, sizeof(pkt));
        
        uint8_t hubChan = pkt.channel;
        if (hubChan > 0 && hubChan <= 13 && hubChan != _lastKnownHubChan) {
            _pendingHubChan = hubChan;
        }

        _lastPacketMs = millis();
        if (!_isConnected) {
            _isConnected = true;
            uint32_t syncTime = millis() - _scanStartMs;
            Serial.printf("[NET] Connection Restored in %ums on Channel %d\n", syncTime, pkt.channel);
            if (_onStatus) _onStatus(true);
        }

        if (pkt.timestamp > 1000000000UL) {
            struct timeval tv = { (time_t)pkt.timestamp, 0 };
            settimeofday(&tv, nullptr);
        }

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

        if (_seqInitialized) {
            uint8_t diff = pkt.seq - _lastSeq;
            if (diff != 1) {
                _dropCount += (uint32_t)(diff - 1);
                DBG_PRINT(Debug::Ch::CH_NETWORK, "Seq drop: last=%u now=%u dropped=%u", _lastSeq, pkt.seq, (uint8_t)(diff - 1));
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
        DBG_PRINT(Debug::Ch::CH_NETWORK, "Unknown type 0x%02X len=%d\n", type, len);
    }
}

/* ============ LOGIC ============ */
/* ========= TRANSMISSION ========= */
void Network::_sendAck(const uint8_t* mac, uint8_t seq) {
    AckPacket pkt = {};
    pkt.type    = PACKET_TYPE_ACK;
    pkt.ack_seq = seq;

    esp_err_t err = esp_now_send(mac, (uint8_t*)&pkt, sizeof(pkt));
    if (err != ESP_OK) {
        DBG_PRINT(Debug::Ch::CH_NETWORK, "ACK TX failed: %d\n", (int)err);
    }
}

/* =============== PUBLIC API =============== */
/* ============ LIFECYCLE ============ */
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
{
    memset(_ssid,      0, sizeof(_ssid));
    memset(_password,  0, sizeof(_password));
    memset(_ntpServer, 0, sizeof(_ntpServer));
    memset(_hubMac,    0, sizeof(_hubMac));
    _instance = this;
}

void Network::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    esp_wifi_set_ps(WIFI_PS_NONE);
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    Serial.println("[NET] Scanning on ALL Channels...");

    if (esp_now_init() != ESP_OK) {
        Serial.println("[NET] ESP-NOW init failed");
        return;
    }
    esp_now_register_recv_cb(_onDataRecv);

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, ESPNOW_BROADCAST, 6);
    peer.channel = 0;
    peer.encrypt = false;
    if (!esp_now_is_peer_exist(ESPNOW_BROADCAST)) esp_now_add_peer(&peer);

    loadConfig();
    Serial.println("[NET] ESP-NOW INIT");
}

void Network::update() {
    uint32_t now = millis();

    if (_pendingHubChan && _pendingHubChan != _lastKnownHubChan) {
        esp_wifi_set_channel(_pendingHubChan, WIFI_SECOND_CHAN_NONE);
        _lastKnownHubChan = _pendingHubChan;
        _pendingHubChan   = 0;
        _seqInitialized   = false;
        Serial.printf("[NET] LOCKED to Hub Channel: %d\n", _lastKnownHubChan);
    }

    if (_isConnected) {
        if (now - _lastPacketMs > HUB_OFFLINE_TIMEOUT_MS) {
            _isConnected = false;
            _lastKnownHubChan = 0;
            _currentScanChan = 13;
            _scanStartMs = now;
            Serial.println("[NET] Connection Lost. Starting Active Scan...");
            if (_onStatus) _onStatus(false);
        }
    } else {
        if (now - _lastHopMs > SCAN_HOP_INTERVAL_MS) {
            _lastHopMs = now;
            _currentScanChan++;
            if (_currentScanChan > 13) _currentScanChan = 1;
            esp_wifi_set_channel(_currentScanChan, WIFI_SECOND_CHAN_NONE);
            DBG_PRINT(Debug::Ch::CH_NETWORK, "Scanning Channel: %d\n", _currentScanChan);
        }
    }
}

/* ============ ACTIONS ============ */
void Network::sendConfig(const char* ssid, const char* password, const char* ntp) {
    ConfigPacket pkt = {};
    pkt.type = PACKET_TYPE_CONFIG;
    strlcpy(pkt.ssid,     ssid,     sizeof(pkt.ssid));
    strlcpy(pkt.password, password, sizeof(pkt.password));
    strlcpy(pkt.ntp,      ntp,      sizeof(pkt.ntp));
    pkt.seq = _txSeq++;

    esp_err_t err = esp_now_send(ESPNOW_BROADCAST, (uint8_t*)&pkt, sizeof(pkt));
    if (err != ESP_OK) {
        DBG_PRINT(Debug::Ch::CH_NETWORK, "Config TX failed: %d\n", (int)err);
    }
}

void Network::sendCmd(uint8_t cmd) {
    CmdPacket pkt = {};
    pkt.type = PACKET_TYPE_CMD;
    pkt.cmd  = cmd;
    pkt.seq  = _txSeq++;
    esp_err_t err = esp_now_send(ESPNOW_BROADCAST, (uint8_t*)&pkt, sizeof(pkt));
    if (err != ESP_OK) {
        DBG_PRINT(Debug::Ch::CH_NETWORK, "Command TX failed: %d\n", (int)err);
    }
}

/* ============ STORAGE ============ */
bool Network::loadConfig() {
    _prefs.begin("ambisense", true);
    String ssid = _prefs.getString("ssid", "");
    if (ssid.length() == 0) {
        _prefs.end();
        return false;
    }
    strlcpy(_ssid,      ssid.c_str(),                                 sizeof(_ssid));
    strlcpy(_password,  _prefs.getString("pass", "").c_str(),         sizeof(_password));
    strlcpy(_ntpServer, _prefs.getString("ntp", "asia.pool.ntp.org").c_str(), sizeof(_ntpServer));
    _prefs.end();
    _hasConfig = true;
    DBG_PRINT(Debug::Ch::CH_NETWORK, "Config loaded: SSID: %s\n", _ssid);
    return true;
}

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

} // namespace AmbiSense::Display