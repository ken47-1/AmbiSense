/* ==================== RTCManager.h ==================== */
#pragma once

/* =============== INCLUDES =============== */
/* ============ PROJECT ============ */
#include "config/Config.h"
#include "config/HardwareConfig.h"

/* ============ THIRD-PARTY ============ */
#include <RTClib.h>
#include <Wire.h>

/* =============== TYPES =============== */
/* ============ ENUMS ============ */
enum class SyncState { IDLE, SYNCING, DONE, FAILED };

/* ============ STRUCTS ============ */
struct NTPStatus {
    SyncState state;
    int       retries;
    bool      everSynced;
};

/* =============== API =============== */
class RTCManager {
/* ============ PUBLIC ============ */
public:
    RTCManager();

    /* ========= LIFECYCLE ========= */
    void begin();
    void update(bool wifiConnected, const char* ntpServer);

    /* ========= GETTERS ========= */
    DateTime  getTime();
    NTPStatus getNTPStatus()   const { return _ntpStatus; }
    bool      isRunning()      const { return _running; }

    /* ========= ACTIONS ========= */
    void forceSync() {
        _ntpStatus.everSynced = false;
        _ntpStatus.state      = SyncState::IDLE;
        _ntpStatus.retries    = 0;
    }

/* ============ PRIVATE ============ */
private:
    /* ========= HANDLERS ========= */
    void _startSync(const char* ntpServer);
    void _finishSync();

    /* ========= STATE ========= */
    RTC_DS3231  _rtc;
    NTPStatus   _ntpStatus;
    bool        _running;
    int         _lastSyncDay;
    uint32_t    _lastSyncCheck;
    uint32_t    _syncStarted;
    bool        _syncInProgress;
    const char* _ntpServer;
};
