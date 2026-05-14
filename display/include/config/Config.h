/* ==================== Config.h ==================== */
/* Shared protocol definitions between the AmbiSense Hub and Display */
#pragma once

/* =============== INCLUDES =============== */
/* ============ CORE ============ */
#include <stdint.h>

/* =============== DEBUG =============== */
#define DEBUG_NETWORK 0
#define DEBUG_GEO     0

/* =============== TIME =============== */
constexpr long     GMT_OFFSET_SEC            = 7 * 3600;        // UTC+7
constexpr int32_t  DAYLIGHT_OFFSET_SEC       = 0;               // No DST
constexpr int32_t  TARGET_SYNC_HOUR          = 12;              // Daily NTP sync hour
constexpr int32_t  TARGET_SYNC_MINUTE        = 0;               // Daily NTP sync minute
constexpr int32_t  NTP_SYNC_TIMEOUT_DELAY_MS = 10000;           // Per-attempt timeout
constexpr int32_t  NTP_SYNC_MAX_RETRIES      = 6;               // Max sync attempts

/* =============== WEATHER =============== */
constexpr uint32_t WEATHER_INTERVAL_MS      = 30 * 60 * 1000;   // Fetch interval
constexpr uint32_t WEATHER_RETRY_DELAY_MS   = 2000;             // Retry backoff base
constexpr uint8_t  WEATHER_MAX_RETRIES      = 3;                // Max fetch retries

/* =============== LOCATION =============== */
constexpr uint32_t LOCATION_RETRY_INTERVAL_MS = 5000;           // Retry interval
constexpr uint32_t LOCATION_MAX_RETRIES       = 10;             // Max retries

/* =============== INTERVALS =============== */
constexpr uint32_t DHT_INTERVAL_MS        = 2000;               // DHT22 poll rate
constexpr uint32_t BROADCAST_INTERVAL_MS  = 250;                // ESP-NOW rate
constexpr uint32_t SCAN_HOP_INTERVAL_MS   = 1000;               // Channel scan rate
constexpr uint32_t SYNC_CHECK_INTERVAL_MS = 1000;               // RTC sync check rate

/* =============== TIMEOUTS =============== */
constexpr uint32_t HUB_OFFLINE_TIMEOUT_MS    = 15000;           // Hub lost detection
constexpr uint32_t STALE_DATA_TIMEOUT_MS     = 5000;            // Weather/room expiry
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS   = 15000;           // WiFi connection timeout
constexpr uint32_t WIFI_RETRY_INTERVAL_MS    = 30000;           // WiFi retry backoff

/* =============== ESP-NOW =============== */
constexpr uint8_t ESPNOW_BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/* =============== PACKET TYPES =============== */
constexpr uint8_t PACKET_TYPE_DATA   = 0x01;
constexpr uint8_t PACKET_TYPE_CONFIG = 0x02;
constexpr uint8_t PACKET_TYPE_ACK    = 0x03;
constexpr uint8_t PACKET_TYPE_CMD    = 0x04;

/* =============== COMMAND IDs =============== */
constexpr uint8_t CMD_FORCE_NTP_SYNC = 0x01;

/* =============== PACKET STRUCTS =============== */
/* ------ Hub -> Display ------ */
struct __attribute__((packed)) DataPacket {
    uint8_t  type;
    uint8_t  seq;
    uint8_t  channel;
    uint8_t  wifiConnected;
    uint32_t timestamp;
    uint8_t  locationValid;
    char     city[33];
    uint8_t  weatherValid;
    uint8_t  weatherCode;
    float    outsideTemp;
    float    apparentTemp;
    uint8_t  outsideHumi;
    uint16_t outsidePress;
    float    windSpeed;
    int16_t  windDirection;
    char     sunrise[8];
    char     sunset[8];
    uint8_t  roomValid;
    float    roomTemp;
    float    roomHumi;
};

static_assert(sizeof(DataPacket) < 240, "DataPacket too large for ESP-NOW");

/* ------ Display -> Hub ------ */
struct __attribute__((packed)) ConfigPacket {
    uint8_t type;
    char    ssid[33];
    char    password[64];
    char    ntp[64];
    uint8_t seq;
};

/* ------ Bidirectional ------ */
struct __attribute__((packed)) AckPacket {
    uint8_t type;
    uint8_t ack_seq;
};

/* ------ Display -> Hub ------ */
struct __attribute__((packed)) CmdPacket {
    uint8_t type;
    uint8_t cmd;
    uint8_t seq;
};