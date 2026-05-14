/* ==================== UI.h ==================== */
#pragma once

/* =============== INCLUDES =============== */
/* ============ PROJECT ============ */
#include "config/Config.h"
#include "WeatherTypes.h"

/* ============ THIRD-PARTY ============ */
#include <lvgl.h>
#include <Preferences.h>

/* ============ CORE ============ */
#include <time.h>

/* =============== TYPES =============== */
/* ============ ENUMS ============ */
enum class Screen     { DASHBOARD, CONFIG };
enum class DateFormat { TEXT, NUMERIC };

/* ============ STRUCTS ============ */
struct Palette {
    uint32_t bg;
    uint32_t text;
    uint32_t text_invert;
    uint32_t subtext;
    uint32_t dim;
    uint32_t unknown;
    uint32_t settings;
    uint32_t red;
    uint32_t location;
    uint32_t deep_orange;
    uint32_t orange;
    uint32_t gold;
    uint32_t green;
    uint32_t wind;
    uint32_t online;
    uint32_t pastel_blue;
    uint32_t sky_blue;
};

/* ============ CONSTANTS ============ */
#define CLK_SIZE 140

/* =============== API =============== */
class UI {
public:
    static const Palette* theme;

    UI();
    void begin();
    void update(const DataPacket& pkt, bool hubOnline);
    void showScreen(Screen s);

    void setOnConfigSubmit(void (*cb)(const char*, const char*, const char*));
    void setOnForceSyncCmd(void (*cb)());

private:
    void _loadPrefs();
    void _savePrefs();

    void _buildDashboard();
    void _buildConfig();
    void _updateDashboard(const DataPacket& pkt, bool hubOnline);
    void _drawAnalogClock(int h, int m, int s);

    lv_obj_t* _makeLabel(lv_obj_t* parent, const char* txt, const lv_font_t* font,
                         uint32_t col, int x, int y);
    void _makeHdiv(lv_obj_t* parent, int x, int y, int w, int thick, uint32_t col);
    void _makeVdiv(lv_obj_t* parent, int x, int y, int h, int thick, uint32_t col);

    lv_obj_t* _scrDashboard;
    lv_obj_t* _scrConfig;
    lv_obj_t* _lblWeatherIcon;
    lv_obj_t* _lblWeatherTemp;
    lv_obj_t* _lblCondition;
    lv_obj_t* _lblLocationIcon;
    lv_obj_t* _lblLocation;
    lv_obj_t* _lblFeelsLike;
    lv_obj_t* _lblHumidIcon;
    lv_obj_t* _lblHumidVal;
    lv_obj_t* _lblPressIcon;
    lv_obj_t* _lblPressVal;
    lv_obj_t* _lblWindIcon;
    lv_obj_t* _lblWindDirVal;
    lv_obj_t* _lblSunriseIcon;
    lv_obj_t* _lblSunriseVal;
    lv_obj_t* _lblSunsetIcon;
    lv_obj_t* _lblSunsetVal;
    lv_obj_t* _lblRoomHeader;
    lv_obj_t* _lblTempIcon;
    lv_obj_t* _lblRoomTempVal;
    lv_obj_t* _lblHumiIcon;
    lv_obj_t* _lblRoomHumidVal;
    lv_obj_t* _lblTime;
    lv_obj_t* _lblDate;
    lv_obj_t* _lblDay;
    lv_obj_t* _canvas;
    lv_obj_t* _statusDot;
    lv_obj_t* _btnSettings;
    lv_obj_t* _tabviewConfig;
    lv_obj_t* _taSSID;
    lv_obj_t* _taPass;
    lv_obj_t* _taNTP;
    lv_obj_t* _kbConfig;
    lv_obj_t* _btnTheme;
    lv_obj_t* _lblTheme;
    lv_obj_t* _swSeconds;
    lv_obj_t* _ddDateFmt;

    Preferences _prefs;
    DateFormat  _dateFmt;
    bool        _showSeconds;
    bool        _darkTheme;
    Screen      _currentScreen;

    uint32_t _lastWeatherValidMs;
    uint32_t _lastRoomValidMs;

    static UI*         _instance;
    static lv_color_t  _clockBuf[CLK_SIZE * CLK_SIZE];

    void (*_onConfigSubmit)(const char*, const char*, const char*);
    void (*_onForceSync)();

    static void _onSettingsBtnCb(lv_event_t* e);
    static void _onThemeBtnCb(lv_event_t* e);
    static void _onSecondsSwitchCb(lv_event_t* e);
    static void _onDateFmtDropdownCb(lv_event_t* e);
    static void _onConfigSaveCb(lv_event_t* e);
    static void _onForceSyncCb(lv_event_t* e);
    static void _onBackBtnCb(lv_event_t* e);
    static void _onTaEvent(lv_event_t* e);
    static void _onKbEvent(lv_event_t* e);
};