/* ========== Config.h ========== */
/* Shared protocol definitions — AmbiSense Hub and Display */
#pragma once

/* ===== INCLUDES ===== */
/* --- CORE --- */
#include <stdint.h>

/* ===== DEBUG ===== */
#define DEBUG_NETWORK 0

/* ===== TIME ===== */
constexpr long     GMT_OFFSET_SEC         = 7 * 3600;   // UTC+7, Bangkok
constexpr int      DAYLIGHT_OFFSET_SEC    = 0;          // No DST in Thailand
constexpr int      TARGET_SYNC_HOUR       = 12;
constexpr int      TARGET_SYNC_MINUTE     = 0;

/* ===== WEATHER ===== */
constexpr uint32_t WEATHER_INTERVAL_MS      = 30 * 60 * 1000;   // 30 minutes
constexpr uint32_t WEATHER_RETRY_DELAY_MS   = 2000;             // Initial backoff: 2s
constexpr uint8_t  WEATHER_MAX_RETRIES      = 3;

/* ===== INTERVALS ===== */
constexpr uint32_t DHT_INTERVAL_MS        = 2000;   // DHT22 min reliable interval
constexpr uint32_t BROADCAST_INTERVAL_MS  = 250;    // ESP-NOW broadcast rate
constexpr uint32_t SCAN_HOP_INTERVAL_MS   = 1000;   // Channel hopping interval
constexpr uint32_t SYNC_CHECK_INTERVAL_MS = 1000;

/* ===== ESP-NOW ===== */
constexpr uint8_t ESPNOW_BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/* ===== PACKET TYPES ===== */
constexpr uint8_t PACKET_TYPE_DATA   = 0x01;
constexpr uint8_t PACKET_TYPE_CONFIG = 0x02;
constexpr uint8_t PACKET_TYPE_ACK    = 0x03;
constexpr uint8_t PACKET_TYPE_CMD    = 0x04;

/* ===== COMMAND IDs ===== */
constexpr uint8_t CMD_FORCE_NTP_SYNC = 0x01;

/* ===== PACKET STRUCTS ===== */

/* --- Hub -> Display --- */
struct __attribute__((packed)) DataPacket {
    /* --- System --- */
    uint8_t type;            // PACKET_TYPE_DATA
    uint8_t seq;             // Rolling sequence number
    uint8_t channel;         // Wi-Fi channel
    uint8_t wifiConnected;   // 1 if Hub is connected to Wi-Fi, 0 otherwise
    
    /* --- RTC --- */
    uint32_t timestamp;   // Unix timestamp from DS3231

    /* --- Weather --- */
    uint8_t weatherValid;      // Weather validity check
    int     weatherCode;
    float   outsideTemp;
    float   apparentTemp;      // "Feels Like" Temperature
    int     outsideHumi;
    int     outsidePressure;
    float   windSpeed;
    int     windDir;
    char    sunrise[6];
    char    sunset[6];

    /* --- Room Metrics --- */
    uint8_t roomValid;   // Room sensors validity check
    float   roomTemp;    // Room temperature, Celsius
    float   roomHumi;    // Room humidity, percent
};

static_assert(sizeof(DataPacket) < 200, "DataPacket too large for ESP-NOW");

/* --- Display -> Hub --- */
struct __attribute__((packed)) ConfigPacket {
    uint8_t type;           // PACKET_TYPE_CONFIG
    char    ssid[33];       // Max 32 chars + null terminator
    char    password[64];   // Max 63 chars + null terminator
    char    ntp[64];        // NTP server string + null terminator
    uint8_t seq;            // Sequence number for ACK matching
};

// Bidirectional
struct __attribute__((packed)) AckPacket {
    uint8_t type;           // PACKET_TYPE_ACK
    uint8_t ack_seq;        // Echoes back seq of received packet
};

// Display -> Hub
struct __attribute__((packed)) CmdPacket {
    uint8_t type;           // PACKET_TYPE_CMD
    uint8_t cmd;            // Command ID (e.g. CMD_FORCE_NTP_SYNC)
    uint8_t seq;            // Sequence number for ACK matching
};
