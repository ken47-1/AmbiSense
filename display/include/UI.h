/* ==================== UI.h ==================== */
#pragma once

/* =============== INCLUDES =============== */
/* ============ PROJECT ============ */
#include "fonts/inconsolata_14.h"
#include "fonts/inconsolata_16.h"
#include "fonts/material_design_other_20.h"
#include "fonts/material_design_weather_40.h"

#include "config/LocationConfig.h"
#include "WeatherTypes.h"

/* ============ THIRD-PARTY ============ */
#include <lvgl.h>
#include <Preferences.h>

/* ============ CORE ============ */
#include <time.h>

/* =============== TYPES =============== */
/* ============ STRUCTS ============ */
struct Palette {
    uint32_t bg, text, subtext, dim, settings;
    uint32_t red, deep_orange, orange, gold;
    uint32_t green, wind, online;
    uint32_t pastel_blue, sky_blue;
};

/* ============ ENUMS ============ */
enum class Screen     { DASHBOARD, CONFIG };
enum class DateFormat { TEXT, NUMERIC };

/* =============== CONFIG =============== */
#define CLK_SIZE 140

/* =============== API =============== */
class UI {
/* ============ PUBLIC ============ */
public:
    static const Palette* theme;

    UI();

    /* ========= LIFECYCLE ========= */
    void begin();

    /* hubOnline = packets arriving within timeout; drives status dot */
    void update(uint32_t timestamp,
                float roomTemp, float roomHumi, bool roomValid,
                const WeatherData& weather,
                bool hubOnline);

    /* ========= ACTIONS ========= */
    void showScreen(Screen s);

    /* ========= CALLBACKS ========= */
    void setOnConfigSubmit(void (*cb)(const char*, const char*, const char*)) { _onConfigSubmit = cb; }
    void setOnForceSyncCmd(void (*cb)()) { _onForceSync = cb; }

/* ============ PRIVATE ============ */
private:
    /* ========= SCREENS ========= */
    lv_obj_t* _scrDashboard;
    lv_obj_t* _scrConfig;

    /* ========= DASHBOARD WIDGETS ========= */
    /* ------ Weather ------ */
    lv_obj_t* _lblWeatherIcon;
    lv_obj_t* _lblWeatherTemp;
    lv_obj_t* _lblCondition;
    lv_obj_t* _lblLocationIcon;
    lv_obj_t* _lblLocation;
    lv_obj_t* _lblFeelsLike;
    lv_obj_t* _lblHumidIcon,  *_lblHumidVal;
    lv_obj_t* _lblPressIcon,  *_lblPressVal;
    lv_obj_t* _lblWindIcon,   *_lblWindValDir;
    lv_obj_t* _lblSRIcon,     *_lblSunriseVal;
    lv_obj_t* _lblSSIcon,     *_lblSunsetVal;

    /* ------ Room Sensors ------ */
    lv_obj_t* _lblRoomHeader;
    lv_obj_t* _lblTempIcon,   *_lblRoomTempVal;
    lv_obj_t* _lblHumiIcon,   *_lblRoomHumidVal;

    /* ------ Clock + Chrome ------ */
    lv_obj_t* _lblTime;
    lv_obj_t* _lblDate;
    lv_obj_t* _lblDay;
    lv_obj_t* _canvas;
    lv_obj_t* _statusDot;
    lv_obj_t* _btnSettings;
    static lv_color_t _clockBuf[CLK_SIZE * CLK_SIZE];

    /* ========= CONFIG WIDGETS ========= */
    /* ------ Navigation ------ */
    lv_obj_t* _tabBtnWifi;
    lv_obj_t* _tabBtnSettings;
    lv_obj_t* _panelWifi;
    lv_obj_t* _panelSettings;

    /* ------ Inputs ------ */
    lv_obj_t* _taSSID;
    lv_obj_t* _taPass;
    lv_obj_t* _taNTP;
    lv_obj_t* _kbConfig;

    /* ------ Settings ------ */
    lv_obj_t* _btnTheme;
    lv_obj_t* _lblTheme;
    lv_obj_t* _swSeconds;
    lv_obj_t* _ddDateFmt;

    /* ========= STATE ========= */
    Preferences _prefs;
    DateFormat  _dateFmt;
    bool        _showSeconds;
    bool        _darkTheme;
    Screen      _currentScreen;
    static UI*  _instance;

    void (*_onConfigSubmit)(const char*, const char*, const char*);
    void (*_onForceSync)();

    /* ========= BUILDERS ========= */
    void _buildDashboard();
    void _buildConfig();
    void _updateDashboard(uint32_t timestamp,
                          float roomTemp, float roomHumi, bool roomValid,
                          const WeatherData& weather);
    void _drawAnalogClock(int h, int m, int s);

    /* ========= UI FACTORY ========= */
    lv_obj_t*  _makeLabel(lv_obj_t* parent, const char* txt, const lv_font_t* font,
                           uint32_t col, int x, int y);
    lv_obj_t*  _makeSpangroup(lv_obj_t* parent, int x, int y, int w, lv_text_align_t align);
    lv_span_t* _addSpan(lv_obj_t* sg, const char* txt, const lv_font_t* font, lv_color_t color);
    void       _makeHdiv(lv_obj_t* parent, int x, int y, int w, int thickness, uint32_t color);
    void       _makeVdiv(lv_obj_t* parent, int x, int y, int h, int thickness, uint32_t color);

    /* Sets flat appearance on a button: same color in all states, no shadow/animation */
    void _makeFlatBtn(lv_obj_t* btn, uint32_t color);

    /* ========= STORAGE ========= */
    void _loadPrefs();
    void _savePrefs();

    /* ========= EVENT CALLBACKS ========= */
    static void _onSettingsBtnCb(lv_event_t* e);
    static void _onTabSwitchCb(lv_event_t* e);
    static void _onThemeBtnCb(lv_event_t* e);
    static void _onSecondsSwitchCb(lv_event_t* e);
    static void _onDateFmtDropdownCb(lv_event_t* e);
    static void _onConfigSaveCb(lv_event_t* e);
    static void _onForceSyncCb(lv_event_t* e);
    static void _onBackBtnCb(lv_event_t* e);
    /* kb passed as user_data; handles FOCUSED/DEFOCUSED to show/hide keyboard */
    static void _onTaEvent(lv_event_t* e);
    /* handles READY/CANCEL on keyboard itself */
    static void _onKbEvent(lv_event_t* e);
};
