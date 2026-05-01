/* ==================== UI.cpp ==================== */
#include "UI.h"
#include <math.h>
#include <string.h>

/* =============== INCLUDES =============== */
/* ============ PROJECT ============ */
#include "fonts/inconsolata_14.h"
#include "fonts/inconsolata_16.h"
#include "fonts/material_design_other_20.h"
#include "fonts/material_design_weather_40.h"

/* ============ THIRD-PARTY ============ */
/* None */

/* ============ CORE ============ */
#include <Arduino.h>
#include <time.h>

/* =============== INTERNAL STATE =============== */
/* ============ LAYOUT CONSTANTS ============ */
#define SCR_W      320
#define SCR_H      240
#define MARGIN      10
#define DIV_MARGIN   5
#define CLK_SIZE   140
#define CLK_R      (CLK_SIZE / 2)
#define CLK_CX     (CLK_SIZE / 2)
#define CLK_CY     (CLK_SIZE / 2)
#define CLK_X      (SCR_W - CLK_SIZE - MARGIN + (CLK_SIZE/2 - CLK_R))
#define CLK_Y      MARGIN
#define VDIV_X     (CLK_X - MARGIN - 2)

#define TAB_H       36
#define TAB_W       (SCR_W / 2)
#define KB_H        120
#define BTN_H       36
#define CONT_H      (SCR_H - TAB_H - BTN_H)

/* ============ PALETTES ============ */
static const Palette DARK = {
    .bg          = 0x000000,
    .text        = 0xFFFFFF,
    .subtext     = 0xAFAFB6,
    .dim         = 0x61616B,
    .settings    = 0x878792,
    .red         = 0xFF2D2D,
    .deep_orange = 0xFF6333,
    .orange      = 0xFF9630,
    .gold        = 0xFFCA28,
    .green       = 0x00e626,
    .wind        = 0x52CC58,
    .online      = 0x2DE636,
    .pastel_blue = 0xA8E0FA,
    .sky_blue    = 0x55C4F7,
};

static const Palette LIGHT = {
    .bg          = 0xF2F2F7,
    .text        = 0x18181b,
    .subtext     = 0x60606c,
    .dim         = 0xafafb6,
    .settings    = 0x878792,
    .red         = 0xFF2D2D,
    .deep_orange = 0xFF6333,
    .orange      = 0xFF9630,
    .gold        = 0xFFCA28,
    .green       = 0x00e626,
    .wind        = 0x52CC58,
    .online      = 0x2DE636,
    .pastel_blue = 0x60C6F6,
    .sky_blue    = 0x0BAAF4,
};

/* ============ STATIC MEMBERS ============ */
const Palette* UI::theme = &DARK;
UI*            UI::_instance = nullptr;
lv_color_t     UI::_clockBuf[CLK_SIZE * CLK_SIZE];

static const char* MONTH_NAMES[] = {
    "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char* DAY_NAMES[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};

/* =============== INTERNAL HELPERS =============== */
/* ============ UI FACTORY ============ */
lv_obj_t* UI::_makeLabel(lv_obj_t* parent, const char* txt, const lv_font_t* font,
                         uint32_t color, int x, int y) {
    lv_obj_t* lb = lv_label_create(parent);
    lv_label_set_text(lb, txt);
    lv_obj_set_style_text_font(lb, font, 0);
    lv_obj_set_style_text_color(lb, lv_color_hex(color), 0);
    lv_obj_set_pos(lb, x, y);
    return lb;
}

void UI::_makeHdiv(lv_obj_t* parent, int x, int y, int w, int thickness, uint32_t color) {
    lv_obj_t* r = lv_obj_create(parent);
    lv_obj_remove_style_all(r);
    lv_obj_set_pos(r, x, y);
    lv_obj_set_size(r, w, thickness);
    lv_obj_set_style_bg_color(r, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(r, LV_OPA_COVER, 0);
    lv_obj_clear_flag(r, LV_OBJ_FLAG_CLICKABLE);
}

void UI::_makeVdiv(lv_obj_t* parent, int x, int y, int h, int thickness, uint32_t color) {
    lv_obj_t* r = lv_obj_create(parent);
    lv_obj_remove_style_all(r);
    lv_obj_set_pos(r, x, y);
    lv_obj_set_size(r, thickness, h);
    lv_obj_set_style_bg_color(r, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(r, LV_OPA_COVER, 0);
    lv_obj_clear_flag(r, LV_OBJ_FLAG_CLICKABLE);
}

/* ============ STORAGE ============ */
void UI::_loadPrefs() {
    _prefs.begin("ui_prefs", true);
    _dateFmt     = _prefs.getBool("dateFmtText", true) ? DateFormat::TEXT : DateFormat::NUMERIC;
    _showSeconds = _prefs.getBool("showSeconds", false);
    _darkTheme   = _prefs.getBool("darkTheme", true);
    _prefs.end();
}

void UI::_savePrefs() {
    _prefs.begin("ui_prefs", false);
    _prefs.putBool("dateFmtText", _dateFmt == DateFormat::TEXT);
    _prefs.putBool("showSeconds", _showSeconds);
    _prefs.putBool("darkTheme", _darkTheme);
    _prefs.end();
}

/* =============== PUBLIC API =============== */
/* ============ LIFECYCLE ============ */
UI::UI()
    : _scrDashboard(nullptr)
    , _scrConfig(nullptr)
    , _lblWeatherIcon(nullptr)
    , _lblWeatherTemp(nullptr)
    , _lblCondition(nullptr)
    , _lblLocationIcon(nullptr)
    , _lblLocation(nullptr)
    , _lblFeelsLike(nullptr)
    , _lblHumidIcon(nullptr)
    , _lblHumidVal(nullptr)
    , _lblPressIcon(nullptr)
    , _lblPressVal(nullptr)
    , _lblWindIcon(nullptr)
    , _lblWindDirVal(nullptr)
    , _lblSunriseIcon(nullptr)
    , _lblSunriseVal(nullptr)
    , _lblSunsetIcon(nullptr)
    , _lblSunsetVal(nullptr)
    , _lblRoomHeader(nullptr)
    , _lblTempIcon(nullptr)
    , _lblRoomTempVal(nullptr)
    , _lblHumiIcon(nullptr)
    , _lblRoomHumidVal(nullptr)
    , _lblTime(nullptr)
    , _lblDate(nullptr)
    , _lblDay(nullptr)
    , _canvas(nullptr)
    , _statusDot(nullptr)
    , _btnSettings(nullptr)
    , _tabBtnWifi(nullptr)
    , _tabBtnSettings(nullptr)
    , _panelWifi(nullptr)
    , _panelSettings(nullptr)
    , _taSSID(nullptr)
    , _taPass(nullptr)
    , _taNTP(nullptr)
    , _kbConfig(nullptr)
    , _btnTheme(nullptr)
    , _lblTheme(nullptr)
    , _swSeconds(nullptr)
    , _ddDateFmt(nullptr)
    , _dateFmt(DateFormat::TEXT)
    , _showSeconds(false)
    , _darkTheme(true)
    , _currentScreen(Screen::DASHBOARD)
    , _onConfigSubmit(nullptr)
    , _onForceSync(nullptr)
{
    _instance = this;
}

void UI::begin() {
    _loadPrefs();
    theme = _darkTheme ? &DARK : &LIGHT;
    _buildDashboard();
    _buildConfig();
    Serial.println("[UI] Initialized.");
}

void UI::update(const DataPacket& pkt, bool hubOnline) {
    if (_currentScreen != Screen::DASHBOARD) return;

    /* Placeholder mode */
    if (pkt.timestamp < 1000000000UL) {
        lv_canvas_fill_bg(_canvas, lv_color_hex(theme->bg), LV_OPA_COVER);
        _drawAnalogClock(0, 0, 0);
        
        const char* t_ph = _showSeconds ? "--:--:--" : "--:--";
        if (strcmp(lv_label_get_text(_lblTime), t_ph) != 0) {
            lv_label_set_text(_lblTime, t_ph);
        }
        
        const char* d_ph = (_dateFmt == DateFormat::TEXT) ? "-- --- ----" : "--/--/----";
        if (strcmp(lv_label_get_text(_lblDate), d_ph) != 0) {
            lv_label_set_text(_lblDate, d_ph);
        }
        return;
    }

    /* Status dot */
    if (_statusDot) {
        lv_color_t targetCol;
        if (!hubOnline) {
            targetCol = lv_color_hex(theme->red);
        } else if (!pkt.weatherValid) {
            targetCol = lv_color_hex(theme->gold);
        } else {
            targetCol = lv_color_hex(theme->online);
        }
        
        lv_color_t currentCol = lv_obj_get_style_bg_color(_statusDot, 0);
        if (targetCol.full != currentCol.full) {
            lv_obj_set_style_bg_color(_statusDot, targetCol, 0);
        }
    }

    _updateDashboard(pkt);
}

void UI::showScreen(Screen s) {
    _currentScreen = s;
    lv_scr_load(s == Screen::DASHBOARD ? _scrDashboard : _scrConfig);
}

void UI::setOnConfigSubmit(void (*cb)(const char*, const char*, const char*)) {
    _onConfigSubmit = cb;
}

void UI::setOnForceSyncCmd(void (*cb)()) {
    _onForceSync = cb;
}

/* ============ SCREEN BUILDERS ============ */
void UI::_buildDashboard() {
    _scrDashboard = lv_obj_create(NULL);
    lv_obj_add_flag(_scrDashboard, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_bg_color(_scrDashboard, lv_color_hex(theme->bg), 0);
    lv_obj_set_style_pad_all(_scrDashboard, 0, 0);
    lv_obj_set_scrollbar_mode(_scrDashboard, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(_scrDashboard, LV_OBJ_FLAG_SCROLLABLE);

    const int fh14 = lv_font_get_line_height(&inconsolata_14);
    const int fh16 = lv_font_get_line_height(&inconsolata_16);
    const int room_top_y = SCR_H - MARGIN - fh14 + 1 - fh16 - MARGIN;

    _makeHdiv(_scrDashboard, DIV_MARGIN, room_top_y, (VDIV_X - DIV_MARGIN) + 2, 2, theme->subtext);
    _makeVdiv(_scrDashboard, VDIV_X, DIV_MARGIN, SCR_H - (DIV_MARGIN * 2), 2, theme->subtext);

    /* Row 1: Weather icon + temp */
    {
    const int icon_gap = 6;
    const int r1_y     = MARGIN - 2;

    _lblWeatherIcon = _makeLabel(_scrDashboard, "\xF3\xB0\x8B\x97", &material_design_weather_40, theme->subtext, 0, r1_y);
    _lblWeatherTemp = _makeLabel(_scrDashboard, "--\xC2\xB0""C", &lv_font_montserrat_32, theme->text, 0, r1_y);
    
    lv_obj_update_layout(_scrDashboard);
    int iw = lv_obj_get_width(_lblWeatherIcon);
    int tw = lv_obj_get_width(_lblWeatherTemp);
    int sx = (VDIV_X - (iw + icon_gap + tw)) / 2;
    lv_obj_set_x(_lblWeatherIcon, sx);
    lv_obj_set_x(_lblWeatherTemp, sx + iw + icon_gap);
    }

    /* Row 2: Weather condition */
    {
    const int r2_y = 46;

    _lblCondition = _makeLabel(_scrDashboard, "Unknown", &lv_font_montserrat_18, theme->text,
        MARGIN, r2_y);
    lv_obj_set_style_text_align(_lblCondition, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(_lblCondition, VDIV_X - (MARGIN * 2));
    }

    /* Row 3: Location */
    {
    const int icon_gap = 2;
    const int r3_y     = 70;

    _lblLocationIcon = _makeLabel(_scrDashboard, "\xF3\xB0\x8D\x8E", &material_design_other_20, theme->red, 0, r3_y + 2);
    _lblLocation = _makeLabel(_scrDashboard, "Unknown", &lv_font_montserrat_18, theme->text, 0, r3_y);
    
    lv_obj_update_layout(_scrDashboard);
    int iw = lv_obj_get_width(_lblLocationIcon);
    int tw = lv_obj_get_width(_lblLocation);
    int sx = (VDIV_X - (iw + icon_gap + tw)) / 2;
    lv_obj_set_x(_lblLocationIcon, sx);
    lv_obj_set_x(_lblLocation, sx + iw + icon_gap);
    }

    /* Row 4: Feels like */
    {
    const int r4_y = 94;

    _lblFeelsLike = _makeLabel(_scrDashboard, "Feels like: --\xC2\xB0""C", &lv_font_montserrat_16, theme->text,
        MARGIN, lv_obj_get_y(_lblLocation) + lv_obj_get_height(_lblLocation) + 6);
    lv_obj_set_width(_lblFeelsLike, VDIV_X - (MARGIN * 2));
    lv_obj_set_style_text_align(_lblFeelsLike, LV_TEXT_ALIGN_CENTER, 0);
    }

    /* Row 5: Humidity + Pressure */
    {
    const int humi_gap    = 0;
    const int press_gap   = 4;
    const int pair_gap    = 10;
    const int r5_x_offset = -4;
    const int r5_y        = 125;
    
    _lblHumidIcon = _makeLabel(_scrDashboard, "\xF3\xB0\x96\x8E", &material_design_other_20, theme->sky_blue, 0, r5_y - 2);
    _lblHumidVal  = _makeLabel(_scrDashboard, "--%", &inconsolata_14, theme->text, 0, r5_y);
    _lblPressIcon = _makeLabel(_scrDashboard, "\xF3\xB0\xA1\xB5", &material_design_other_20, theme->pastel_blue, 0, r5_y - 2);
    _lblPressVal  = _makeLabel(_scrDashboard, "---- hPa", &inconsolata_14, theme->text, 0, r5_y);
    
    lv_obj_update_layout(_scrDashboard);
    int hiw = lv_obj_get_width(_lblHumidIcon);
    int hvw = lv_obj_get_width(_lblHumidVal);
    int piw = lv_obj_get_width(_lblPressIcon);
    int pvw = lv_obj_get_width(_lblPressVal);
    int total_w = hiw + humi_gap + hvw + pair_gap + piw + press_gap + pvw;
    int sx = (VDIV_X - total_w) / 2;
    sx += r5_x_offset;
    lv_obj_set_x(_lblHumidIcon, sx);
    lv_obj_set_x(_lblHumidVal,  sx + hiw + humi_gap);
    lv_obj_set_x(_lblPressIcon, sx + hiw + humi_gap + hvw + pair_gap);
    lv_obj_set_x(_lblPressVal,  sx + hiw + humi_gap + hvw + pair_gap + piw + press_gap);
    }

    /* Row 6: Wind Speed & Direction */
    {
    const int wind_gap    = 4;
    const int r6_x_offset = -3;
    const int r6_y        = 144;

    _lblWindIcon = _makeLabel(_scrDashboard, "\xF3\xB0\x96\x9D", &material_design_other_20, theme->wind, 0, r6_y - 1);
    _lblWindDirVal = _makeLabel(_scrDashboard, "-- km/h --", &inconsolata_16, theme->text, 0, r6_y);
    
    lv_obj_update_layout(_scrDashboard);
    int iw = lv_obj_get_width(_lblWindIcon);
    int tw = lv_obj_get_width(_lblWindDirVal);
    int sx = (VDIV_X - (iw + wind_gap + tw)) / 2;
    sx += r6_x_offset;
    lv_obj_set_x(_lblWindIcon, sx);
    lv_obj_set_x(_lblWindDirVal, sx + iw + wind_gap);
    }

    /* Row 7: Sunrise & Sunset */
    {
    const int r7_y         = 165;
    const int sun_gap      = 4;
    const int sun_pair_gap = 10;
    const int r7_x_offset  = -2;

    _lblSunriseIcon = _makeLabel(_scrDashboard, "\xF3\xB0\x96\x9C", &material_design_other_20, theme->gold, 0, r7_y - 1);
    _lblSunriseVal = _makeLabel(_scrDashboard, "--:--", &inconsolata_16, theme->text, 0, r7_y);
    _lblSunsetIcon = _makeLabel(_scrDashboard, "\xF3\xB0\x96\x9B", &material_design_other_20, theme->deep_orange, 0, r7_y - 1);
    _lblSunsetVal = _makeLabel(_scrDashboard, "--:--", &inconsolata_16, theme->text, 0, r7_y);
    
    lv_obj_update_layout(_scrDashboard);
    int riw = lv_obj_get_width(_lblSunriseIcon);
    int rvw = lv_obj_get_width(_lblSunriseVal);
    int siw = lv_obj_get_width(_lblSunsetIcon);
    int svw = lv_obj_get_width(_lblSunsetVal);
    int total_w = riw + sun_gap + rvw + sun_pair_gap + siw + sun_gap + svw;
    int sx = (VDIV_X - total_w) / 2;
    sx += r7_x_offset;
    lv_obj_set_x(_lblSunriseIcon, sx);
    lv_obj_set_x(_lblSunriseVal, sx + riw + sun_gap);
    lv_obj_set_x(_lblSunsetIcon, sx + riw + sun_gap + rvw + sun_pair_gap);
    lv_obj_set_x(_lblSunsetVal, sx + riw + sun_gap + rvw + sun_pair_gap + siw + sun_gap);
    }

    /* Row 8: Room sensors */
    const int r8_header_y = 195;
    const int r8_y        = 217;
    const int temp_gap    = 0;
    const int hum_gap     = 0;
    const int r8_pair_gap = 11;
    const int r8_x_offset = -4;
    
    _lblTempIcon = _makeLabel(_scrDashboard, "\xF3\xB0\x94\x8F", &material_design_other_20, theme->deep_orange, 0, r8_y - 2);
    _lblRoomTempVal = _makeLabel(_scrDashboard, "--.-\xC2\xB0""C", &inconsolata_16, theme->text, 0, r8_y);
    _lblHumiIcon = _makeLabel(_scrDashboard, "\xF3\xB0\x96\x8E", &material_design_other_20, theme->sky_blue, 0, r8_y - 2);
    _lblRoomHumidVal = _makeLabel(_scrDashboard, "--.-%", &inconsolata_16, theme->text, 0, r8_y);
    
    lv_obj_update_layout(_scrDashboard);
    int tiw = lv_obj_get_width(_lblTempIcon);
    int tvw = lv_obj_get_width(_lblRoomTempVal);
    int hiiw = lv_obj_get_width(_lblHumiIcon);
    int hvhw = lv_obj_get_width(_lblRoomHumidVal);
    int total_w = tiw + temp_gap + tvw + r8_pair_gap + hiiw + hum_gap + hvhw;
    int sx = (VDIV_X - total_w) / 2;
    sx += r8_x_offset;
    lv_obj_set_x(_lblTempIcon, sx);
    lv_obj_set_x(_lblRoomTempVal, sx + tiw + temp_gap);
    lv_obj_set_x(_lblHumiIcon, sx + tiw + temp_gap + tvw + r8_pair_gap);
    lv_obj_set_x(_lblRoomHumidVal, sx + tiw + temp_gap + tvw + r8_pair_gap + hiiw + hum_gap);

    _lblRoomHeader = _makeLabel(_scrDashboard, "ROOM METRICS", &inconsolata_16, theme->text,
        MARGIN, lv_obj_get_y(_lblRoomTempVal) - fh16 - 2);

    /* Clock */
    _canvas = lv_canvas_create(_scrDashboard);
    lv_canvas_set_buffer(_canvas, _clockBuf, CLK_SIZE, CLK_SIZE, LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_style_pad_all(_canvas, 0, 0);
    lv_obj_set_style_border_width(_canvas, 0, 0);
    lv_obj_set_pos(_canvas, CLK_X, CLK_Y);

    _lblTime = _makeLabel(_scrDashboard, _showSeconds ? "--:--:--" : "--:--",
        &lv_font_montserrat_24, theme->text, 170, 159);
    lv_obj_set_style_text_align(_lblTime, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(_lblTime, CLK_SIZE);

    _lblDate = _makeLabel(_scrDashboard, (_dateFmt == DateFormat::TEXT) ? "-- --- ----" : "--/--/----",
        &lv_font_montserrat_16, theme->text, 170, 189);
    lv_obj_set_style_text_align(_lblDate, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(_lblDate, CLK_SIZE);

    _lblDay = _makeLabel(_scrDashboard, "------",
        &lv_font_montserrat_14, theme->text, 170, 208);
    lv_obj_set_style_text_align(_lblDay, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(_lblDay, CLK_SIZE);

    _statusDot = lv_obj_create(_scrDashboard);
    lv_obj_remove_style_all(_statusDot);
    lv_obj_set_size(_statusDot, 16, 16);
    lv_obj_set_style_radius(_statusDot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(_statusDot, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(_statusDot, lv_color_hex(theme->dim), 0);
    lv_obj_align(_statusDot, LV_ALIGN_TOP_RIGHT, -8, 8);

    _btnSettings = lv_label_create(_scrDashboard);
    lv_label_set_text(_btnSettings, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_font(_btnSettings, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(_btnSettings, lv_color_hex(theme->settings), 0);
    lv_obj_align(_btnSettings, LV_ALIGN_BOTTOM_RIGHT, -8, -7);
    lv_obj_add_flag(_btnSettings, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(_btnSettings, _onSettingsBtnCb, LV_EVENT_CLICKED, this);

    Serial.println("[UI] Dashboard built.");
    lv_scr_load(_scrDashboard);
}

void UI::_updateDashboard(const DataPacket& pkt) {
    char buf[32];
    WeatherInfo w = pkt.weatherValid ? getWeatherInfo(pkt.weatherCode) : UNKNOWN_WEATHER;

    /* Row 1: Weather icon + temp */
    snprintf(buf, sizeof(buf), pkt.weatherValid ? "%.0f°C" : "--°C", pkt.outsideTemp);
    lv_label_set_text(_lblWeatherTemp, buf);
    lv_label_set_text(_lblWeatherIcon, w.icon);
    lv_obj_set_style_text_color(_lblWeatherIcon, lv_color_hex(w.color), 0);
    lv_obj_update_layout(_scrDashboard);
    int iw = lv_obj_get_width(_lblWeatherIcon);
    int tw = lv_obj_get_width(_lblWeatherTemp);
    int sx = (VDIV_X - (iw + 6 + tw)) / 2;
    lv_obj_set_x(_lblWeatherIcon, sx);
    lv_obj_set_x(_lblWeatherTemp, sx + iw + 6);

    /* Row 2: Weather condition */
    lv_label_set_text(_lblCondition, w.desc);

    /* Row 3: Location */
    if (pkt.locationValid && strlen(pkt.city) > 0) {
        lv_label_set_text(_lblLocation, pkt.city);
        lv_obj_set_style_text_color(_lblLocationIcon, lv_color_hex(theme->red), 0);
    } else {
        lv_label_set_text(_lblLocation, "Unknown");
        lv_obj_set_style_text_color(_lblLocationIcon, lv_color_hex(theme->subtext), 0);
    }
    lv_obj_update_layout(_scrDashboard);
    iw = lv_obj_get_width(_lblLocationIcon);
    tw = lv_obj_get_width(_lblLocation);
    sx = (VDIV_X - (iw + 2 + tw)) / 2;
    lv_obj_set_x(_lblLocationIcon, sx);
    lv_obj_set_x(_lblLocation, sx + iw + 2);

    /* Row 4: Feels like */
    snprintf(buf, sizeof(buf), pkt.weatherValid ? "Feels like: %.0f°C" : "Feels like: --°C", pkt.apparentTemp);
    lv_label_set_text(_lblFeelsLike, buf);

    /* Row 5: Humidity + Pressure */
    snprintf(buf, sizeof(buf), pkt.weatherValid ? "%d%%" : "--%", pkt.outsideHumi);
    lv_label_set_text(_lblHumidVal, buf);
    snprintf(buf, sizeof(buf), pkt.weatherValid ? "%d hPa" : "---- hPa", pkt.outsidePress);
    lv_label_set_text(_lblPressVal, buf);
    lv_obj_update_layout(_scrDashboard);
    int hiw = lv_obj_get_width(_lblHumidIcon);
    int hvw = lv_obj_get_width(_lblHumidVal);
    int piw = lv_obj_get_width(_lblPressIcon);
    int pvw = lv_obj_get_width(_lblPressVal);
    int total_hp = hiw + 0 + hvw + 10 + piw + 4 + pvw;
    sx = (VDIV_X - total_hp) / 2;
    lv_obj_set_x(_lblHumidIcon, sx);
    lv_obj_set_x(_lblHumidVal, sx + hiw + 0);
    lv_obj_set_x(_lblPressIcon, sx + hiw + 0 + hvw + 10);
    lv_obj_set_x(_lblPressVal, sx + hiw + 0 + hvw + 10 + piw + 4);

    /* Row 6: Wind Speed & Direction */
    snprintf(buf, sizeof(buf), pkt.weatherValid ? "%.0f km/h %s" : "-- km/h --",
             pkt.windSpeed, getWindDirection(pkt.windDirection));
    lv_label_set_text(_lblWindDirVal, buf);
    lv_obj_update_layout(_scrDashboard);
    iw = lv_obj_get_width(_lblWindIcon);
    tw = lv_obj_get_width(_lblWindDirVal);
    sx = (VDIV_X - (iw + 4 + tw)) / 2;
    lv_obj_set_x(_lblWindIcon, sx);
    lv_obj_set_x(_lblWindDirVal, sx + iw + 4);

    /* Row 7: Sunrise & Sunset */
    lv_label_set_text(_lblSunriseVal, pkt.weatherValid ? pkt.sunrise : "--:--");
    lv_label_set_text(_lblSunsetVal, pkt.weatherValid ? pkt.sunset : "--:--");
    lv_obj_update_layout(_scrDashboard);
    int riw = lv_obj_get_width(_lblSunriseIcon);
    int rvw = lv_obj_get_width(_lblSunriseVal);
    int siw = lv_obj_get_width(_lblSunsetIcon);
    int svw = lv_obj_get_width(_lblSunsetVal);
    int total_sun = riw + 4 + rvw + 10 + siw + 4 + svw;
    sx = (VDIV_X - total_sun) / 2;
    lv_obj_set_x(_lblSunriseIcon, sx);
    lv_obj_set_x(_lblSunriseVal, sx + riw + 4);
    lv_obj_set_x(_lblSunsetIcon, sx + riw + 4 + rvw + 10);
    lv_obj_set_x(_lblSunsetVal, sx + riw + 4 + rvw + 10 + siw + 4);

    /* Row 8: Room sensors */
    if (pkt.roomValid) {
        snprintf(buf, sizeof(buf), "%.1f°C", pkt.roomTemp);
        lv_label_set_text(_lblRoomTempVal, buf);
        snprintf(buf, sizeof(buf), "%.1f%%", pkt.roomHumi);
        lv_label_set_text(_lblRoomHumidVal, buf);
    } else {
        lv_label_set_text(_lblRoomTempVal, "--.-°C");
        lv_label_set_text(_lblRoomHumidVal, "--.-%");
    }
    lv_obj_update_layout(_scrDashboard);
    int tiw = lv_obj_get_width(_lblTempIcon);
    int tvw = lv_obj_get_width(_lblRoomTempVal);
    int hiiw = lv_obj_get_width(_lblHumiIcon);
    int hvhw = lv_obj_get_width(_lblRoomHumidVal);
    int total_room = tiw + 0 + tvw + 11 + hiiw + 0 + hvhw;
    sx = (VDIV_X - total_room) / 2;
    lv_obj_set_x(_lblTempIcon, sx);
    lv_obj_set_x(_lblRoomTempVal, sx + tiw + 0);
    lv_obj_set_x(_lblHumiIcon, sx + tiw + 0 + tvw + 11);
    lv_obj_set_x(_lblRoomHumidVal, sx + tiw + 0 + tvw + 11 + hiiw + 0);

    /* Clock */
    lv_canvas_fill_bg(_canvas, lv_color_hex(theme->bg), LV_OPA_COVER);
    time_t now = time(nullptr);
    if (now > 1000000000L) {
        struct tm* t = localtime(&now);
        if (_showSeconds) {
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
        } else {
            snprintf(buf, sizeof(buf), "%02d:%02d", t->tm_hour, t->tm_min);
        }
        lv_label_set_text(_lblTime, buf);
        if (_dateFmt == DateFormat::TEXT) {
            snprintf(buf, sizeof(buf), "%02d %s %04d", t->tm_mday, MONTH_NAMES[t->tm_mon + 1], t->tm_year + 1900);
        } else {
            snprintf(buf, sizeof(buf), "%02d/%02d/%04d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);
        }
        lv_label_set_text(_lblDate, buf);
        lv_label_set_text(_lblDay, DAY_NAMES[t->tm_wday]);
        _drawAnalogClock(t->tm_hour, t->tm_min, t->tm_sec);
    }
}

void UI::_drawAnalogClock(int h, int m, int s) {
    lv_canvas_fill_bg(_canvas, lv_color_hex(theme->bg), LV_OPA_COVER);

    lv_draw_arc_dsc_t arc;
    lv_draw_arc_dsc_init(&arc);
    arc.color = lv_color_hex(theme->text);
    arc.width = 2;
    arc.rounded = 1;
    lv_canvas_draw_arc(_canvas, CLK_CX, CLK_CY, CLK_R, 0, 360, &arc);
    
    arc.color = lv_color_hex(theme->dim);
    arc.width = 1;
    lv_canvas_draw_arc(_canvas, CLK_CX, CLK_CY, CLK_R - 3, 0, 360, &arc);

    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    for (int i = 0; i < 60; i++) {
        float a = i * 6.0f * (float)M_PI / 180.0f;
        bool is_hour = (i % 5 == 0);
        bool is_qtr = (i % 15 == 0);
        int outer = CLK_R - 4;
        int inner = is_qtr ? CLK_R - 14 : is_hour ? CLK_R - 11 : CLK_R - 8;
        line.width = is_hour ? 2 : 1;
        line.color = is_hour ? lv_color_hex(theme->text) : lv_color_hex(theme->subtext);
        lv_point_t pts[2] = {
            { (lv_coord_t)(CLK_CX + outer * sinf(a)), (lv_coord_t)(CLK_CY - outer * cosf(a)) },
            { (lv_coord_t)(CLK_CX + inner * sinf(a)), (lv_coord_t)(CLK_CY - inner * cosf(a)) }
        };
        lv_canvas_draw_line(_canvas, pts, 2, &line);
    }

    lv_draw_label_dsc_t l_dsc;
    lv_draw_label_dsc_init(&l_dsc);
    l_dsc.font = &inconsolata_16;
    l_dsc.color = lv_color_hex(theme->text);
    const int NUM_R = CLK_R - 22;
    const int NUM_Y_NUDGE = 1;

    for (int i = 1; i <= 12; i++) {
        char num_buf[3];
        snprintf(num_buf, sizeof(num_buf), "%d", i);
        float a = i * 30.0f * (float)M_PI / 180.0f;
        int nx = (int)(CLK_CX + NUM_R * sinf(a));
        int ny = (int)(CLK_CY + NUM_Y_NUDGE - NUM_R * cosf(a));

        switch (i) {
            case 1:  nx += 1; break;
            case 2:  nx += 1; break;
            case 3:  nx += 1; break;
            case 9:  nx -= 1; break;
            default: break;
        }

        lv_point_t size;
        lv_txt_get_size(&size, num_buf, l_dsc.font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        lv_canvas_draw_text(_canvas, nx - (size.x / 2), ny - (size.y / 2), size.x, &l_dsc, num_buf);
    }

    /* Hour hand */
    float ha = ((h % 12) * 60 + m) / 720.0f * 2.0f * (float)M_PI;
    line.color = lv_color_hex(theme->text);
    line.width = 6;
    lv_point_t hour_pts[2] = {
        { (lv_coord_t)CLK_CX, (lv_coord_t)CLK_CY },
        { (lv_coord_t)(CLK_CX + CLK_R * 0.4f * sinf(ha)), (lv_coord_t)(CLK_CY - CLK_R * 0.4f * cosf(ha)) }
    };
    lv_canvas_draw_line(_canvas, hour_pts, 2, &line);

    /* Minute hand */
    float ma = (m * 60 + s) / 3600.0f * 2.0f * (float)M_PI;
    line.color = lv_color_hex(theme->orange);
    line.width = 4;
    lv_point_t min_pts[2] = {
        { (lv_coord_t)CLK_CX, (lv_coord_t)CLK_CY },
        { (lv_coord_t)(CLK_CX + CLK_R * 0.55f * sinf(ma)), (lv_coord_t)(CLK_CY - CLK_R * 0.55f * cosf(ma)) }
    };
    lv_canvas_draw_line(_canvas, min_pts, 2, &line);

    /* Second hand */
    float sa = s / 60.0f * 2.0f * (float)M_PI;
    line.color = lv_color_hex(theme->red);
    line.width = 2;
    lv_point_t sec_pts[2] = {
        { (lv_coord_t)(CLK_CX - CLK_R * 0.2f * sinf(sa)), (lv_coord_t)(CLK_CY + CLK_R * 0.2f * cosf(sa)) },
        { (lv_coord_t)(CLK_CX + CLK_R * 0.7f * sinf(sa)), (lv_coord_t)(CLK_CY - CLK_R * 0.7f * cosf(sa)) }
    };
    lv_canvas_draw_line(_canvas, sec_pts, 2, &line);

    /* Center dot */
    lv_draw_rect_dsc_t dot;
    lv_draw_rect_dsc_init(&dot);
    dot.bg_color = lv_color_hex(theme->red);
    dot.radius = LV_RADIUS_CIRCLE;
    lv_canvas_draw_rect(_canvas, CLK_CX - 4, CLK_CY - 4, 8, 8, &dot);
}

void UI::_buildConfig() {
    _scrConfig = lv_obj_create(NULL);
    lv_obj_set_size(_scrConfig, SCR_W, SCR_H);
    lv_obj_set_style_bg_color(_scrConfig, lv_color_hex(theme->bg), 0);
    lv_obj_set_style_bg_opa(_scrConfig, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(_scrConfig, 0, 0);
    lv_obj_set_style_radius(_scrConfig, 0, 0);
    lv_obj_clear_flag(_scrConfig, LV_OBJ_FLAG_SCROLLABLE);

    /* Tab buttons */
    _tabBtnWifi = lv_btn_create(_scrConfig);
    lv_obj_set_pos(_tabBtnWifi, 0, 0);
    lv_obj_set_size(_tabBtnWifi, TAB_W, TAB_H);
    lv_obj_set_style_radius(_tabBtnWifi, 0, 0);
    lv_obj_set_style_anim_time(_tabBtnWifi, 0, 0);
    lv_obj_set_style_bg_color(_tabBtnWifi, lv_color_hex(theme->sky_blue), 0);
    lv_obj_set_style_pad_all(_tabBtnWifi, 0, 0);
    lv_obj_t* wifi_label = lv_label_create(_tabBtnWifi);
    lv_label_set_text(wifi_label, "Wi-Fi / NTP");
    lv_obj_set_style_text_font(wifi_label, &lv_font_montserrat_14, 0);
    lv_obj_center(wifi_label);

    _tabBtnSettings = lv_btn_create(_scrConfig);
    lv_obj_set_pos(_tabBtnSettings, TAB_W, 0);
    lv_obj_set_size(_tabBtnSettings, TAB_W, TAB_H);
    lv_obj_set_style_radius(_tabBtnSettings, 0, 0);
    lv_obj_set_style_anim_time(_tabBtnSettings, 0, 0);
    lv_obj_set_style_bg_color(_tabBtnSettings, lv_color_hex(theme->dim), 0);
    lv_obj_set_style_pad_all(_tabBtnSettings, 0, 0);
    lv_obj_t* settings_label = lv_label_create(_tabBtnSettings);
    lv_label_set_text(settings_label, "Settings");
    lv_obj_set_style_text_font(settings_label, &lv_font_montserrat_14, 0);
    lv_obj_center(settings_label);

    lv_obj_add_event_cb(_tabBtnWifi, _onTabSwitchCb, LV_EVENT_CLICKED, this);
    lv_obj_add_event_cb(_tabBtnSettings, _onTabSwitchCb, LV_EVENT_CLICKED, this);

    /* Wi-Fi panel */
    _panelWifi = lv_obj_create(_scrConfig);
    lv_obj_set_pos(_panelWifi, 0, TAB_H);
    lv_obj_set_size(_panelWifi, SCR_W, SCR_H - TAB_H);
    lv_obj_set_style_bg_color(_panelWifi, lv_color_hex(theme->bg), 0);
    lv_obj_set_style_bg_opa(_panelWifi, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_panelWifi, 0, 0);
    lv_obj_set_style_pad_all(_panelWifi, 0, 0);
    lv_obj_set_style_radius(_panelWifi, 0, 0);
    lv_obj_set_style_clip_corner(_panelWifi, false, 0);
    lv_obj_add_flag(_panelWifi, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_clear_flag(_panelWifi, LV_OBJ_FLAG_SCROLLABLE);

    /* Scrollable container */
    lv_obj_t* cont = lv_obj_create(_panelWifi);
    lv_obj_set_pos(cont, 0, 0);
    lv_obj_set_size(cont, SCR_W, CONT_H);
    lv_obj_set_style_radius(cont, 0, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(theme->bg), 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_left(cont, 8, 0);
    lv_obj_set_style_pad_right(cont, 8, 0);
    lv_obj_set_style_pad_top(cont, 4, 0);
    lv_obj_set_style_pad_bottom(cont, 120, 0);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(cont, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_AUTO);

    auto mkLbl = [&](lv_obj_t* parent, const char* txt, int y) {
        lv_obj_t* l = lv_label_create(parent);
        lv_label_set_text(l, txt);
        lv_obj_set_style_text_color(l, lv_color_hex(theme->subtext), 0);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(l, 0, y);
    };
    
    auto mkTa = [&](lv_obj_t* parent, int y, bool pw) -> lv_obj_t* {
        lv_obj_t* ta = lv_textarea_create(parent);
        lv_textarea_set_one_line(ta, true);
        if (pw) lv_textarea_set_password_mode(ta, true);
        lv_obj_set_pos(ta, 0, y);
        lv_obj_set_size(ta, SCR_W - 16, 36);
        lv_obj_set_style_text_font(ta, &lv_font_montserrat_14, 0);
        lv_obj_clear_flag(ta, LV_OBJ_FLAG_SCROLLABLE);
        return ta;
    };

    mkLbl(cont, "SSID:", 0);
    _taSSID = mkTa(cont, 18, false);
    lv_textarea_set_placeholder_text(_taSSID, "Network name");

    mkLbl(cont, "Password:", 64);
    _taPass = mkTa(cont, 82, true);
    lv_textarea_set_placeholder_text(_taPass, "Password");

    mkLbl(cont, "NTP Server:", 128);
    _taNTP = mkTa(cont, 146, false);
    lv_textarea_set_placeholder_text(_taNTP, "NTP Server");
    lv_textarea_set_text(_taNTP, "asia.pool.ntp.org");

    /* Buttons */
    lv_obj_t* btnSave = lv_btn_create(_panelWifi);
    lv_obj_set_pos(btnSave, 0, CONT_H);
    lv_obj_set_size(btnSave, 104, BTN_H);
    lv_obj_set_style_radius(btnSave, 0, 0);
    lv_obj_set_style_bg_color(btnSave, lv_color_hex(theme->sky_blue), 0);
    lv_obj_t* save_label = lv_label_create(btnSave);
    lv_label_set_text(save_label, "Save & Send");
    lv_obj_set_style_text_font(save_label, &lv_font_montserrat_14, 0);
    lv_obj_center(save_label);
    lv_obj_add_event_cb(btnSave, _onConfigSaveCb, LV_EVENT_CLICKED, this);

    lv_obj_t* btnSync = lv_btn_create(_panelWifi);
    lv_obj_set_pos(btnSync, 104, CONT_H);
    lv_obj_set_size(btnSync, 112, BTN_H);
    lv_obj_set_style_radius(btnSync, 0, 0);
    lv_obj_set_style_bg_color(btnSync, lv_color_hex(theme->wind), 0);
    lv_obj_t* sync_label = lv_label_create(btnSync);
    lv_label_set_text(sync_label, "Force NTP");
    lv_obj_set_style_text_font(sync_label, &lv_font_montserrat_14, 0);
    lv_obj_center(sync_label);
    lv_obj_add_event_cb(btnSync, _onForceSyncCb, LV_EVENT_CLICKED, this);

    lv_obj_t* btnBackW = lv_btn_create(_panelWifi);
    lv_obj_set_pos(btnBackW, 216, CONT_H);
    lv_obj_set_size(btnBackW, 104, BTN_H);
    lv_obj_set_style_radius(btnBackW, 0, 0);
    lv_obj_set_style_bg_color(btnBackW, lv_color_hex(theme->dim), 0);
    lv_obj_t* back_label = lv_label_create(btnBackW);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(back_label, &lv_font_montserrat_14, 0);
    lv_obj_center(back_label);
    lv_obj_add_event_cb(btnBackW, _onBackBtnCb, LV_EVENT_CLICKED, this);

    /* Settings panel */
    _panelSettings = lv_obj_create(_scrConfig);
    lv_obj_set_pos(_panelSettings, 0, TAB_H);
    lv_obj_set_size(_panelSettings, SCR_W, SCR_H - TAB_H);
    lv_obj_set_style_radius(_panelSettings, 0, 0);
    lv_obj_set_style_bg_color(_panelSettings, lv_color_hex(theme->bg), 0);
    lv_obj_set_style_bg_opa(_panelSettings, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_panelSettings, 0, 0);
    lv_obj_set_style_pad_all(_panelSettings, 0, 0);
    lv_obj_clear_flag(_panelSettings, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_panelSettings, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t* sp = lv_obj_create(_panelSettings);
    lv_obj_set_pos(sp, 0, 0);
    lv_obj_set_size(sp, SCR_W, SCR_H - TAB_H - BTN_H);
    lv_obj_set_style_radius(sp, 0, 0);
    lv_obj_set_style_bg_color(sp, lv_color_hex(theme->bg), 0);
    lv_obj_set_style_bg_opa(sp, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sp, 0, 0);
    lv_obj_set_style_pad_all(sp, 12, 0);
    lv_obj_clear_flag(sp, LV_OBJ_FLAG_SCROLLABLE);

    auto mkRow = [&](const char* txt, int y) {
        lv_obj_t* l = lv_label_create(sp);
        lv_label_set_text(l, txt);
        lv_obj_set_style_text_color(l, lv_color_hex(theme->text), 0);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(l, 0, y + 6);
    };

    /* Theme */
    mkRow("Theme", 0);
    _btnTheme = lv_btn_create(sp);
    lv_obj_set_pos(_btnTheme, SCR_W - 24 - 90, 0);
    lv_obj_set_size(_btnTheme, 90, 30);
    lv_obj_set_style_bg_color(_btnTheme, lv_color_hex(theme->dim), 0);
    _lblTheme = lv_label_create(_btnTheme);
    lv_label_set_text(_lblTheme, _darkTheme ? "Dark" : "Light");
    lv_obj_set_style_text_font(_lblTheme, &lv_font_montserrat_14, 0);
    lv_obj_center(_lblTheme);
    lv_obj_add_event_cb(_btnTheme, _onThemeBtnCb, LV_EVENT_CLICKED, this);

    /* Show seconds */
    mkRow("Show seconds", 44);
    _swSeconds = lv_switch_create(sp);
    lv_obj_set_pos(_swSeconds, SCR_W - 24 - 50, 44);
    lv_obj_set_size(_swSeconds, 50, 26);
    if (_showSeconds) lv_obj_add_state(_swSeconds, LV_STATE_CHECKED);
    lv_obj_add_event_cb(_swSeconds, _onSecondsSwitchCb, LV_EVENT_VALUE_CHANGED, this);

    /* Date format */
    mkRow("Date format", 88);
    _ddDateFmt = lv_dropdown_create(sp);
    lv_dropdown_set_options(_ddDateFmt, "12 Mar 2026\n12/03/2026");
    lv_dropdown_set_selected(_ddDateFmt, _dateFmt == DateFormat::TEXT ? 0 : 1);
    lv_obj_set_pos(_ddDateFmt, SCR_W - 24 - 130, 83);
    lv_obj_set_width(_ddDateFmt, 130);
    lv_obj_set_style_text_font(_ddDateFmt, &lv_font_montserrat_14, 0);
    lv_obj_add_event_cb(_ddDateFmt, _onDateFmtDropdownCb, LV_EVENT_VALUE_CHANGED, this);

    /* Back button */
    lv_obj_t* btnBackS = lv_btn_create(_panelSettings);
    lv_obj_set_pos(btnBackS, SCR_W - 104, SCR_H - TAB_H - BTN_H);
    lv_obj_set_size(btnBackS, 104, BTN_H);
    lv_obj_set_style_radius(btnBackS, 0, 0);
    lv_obj_set_style_bg_color(btnBackS, lv_color_hex(theme->dim), 0);
    lv_obj_t* back_label_s = lv_label_create(btnBackS);
    lv_label_set_text(back_label_s, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(back_label_s, &lv_font_montserrat_14, 0);
    lv_obj_center(back_label_s);
    lv_obj_add_event_cb(btnBackS, _onBackBtnCb, LV_EVENT_CLICKED, this);

    /* Keyboard */
    _kbConfig = lv_keyboard_create(_panelWifi);
    lv_obj_set_size(_kbConfig, SCR_W, KB_H);
    lv_obj_align(_kbConfig, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(_kbConfig, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_event_cb(_taSSID, _onTaEvent, LV_EVENT_ALL, _kbConfig);
    lv_obj_add_event_cb(_taPass, _onTaEvent, LV_EVENT_ALL, _kbConfig);
    lv_obj_add_event_cb(_taNTP,  _onTaEvent, LV_EVENT_ALL, _kbConfig);
}

/* =============== STATIC CALLBACKS =============== */
void UI::_onSettingsBtnCb(lv_event_t* e) {
    UI* ui = (UI*)lv_event_get_user_data(e);
    ui->showScreen(Screen::CONFIG);
}

void UI::_onTabSwitchCb(lv_event_t* e) {
    UI* ui = (UI*)lv_event_get_user_data(e);
    bool isWifi = (lv_event_get_target(e) == ui->_tabBtnWifi);

    if (ui->_kbConfig) {
        lv_keyboard_set_textarea(ui->_kbConfig, nullptr);
        lv_obj_add_flag(ui->_kbConfig, LV_OBJ_FLAG_HIDDEN);
    }

    if (isWifi) {
        lv_obj_clear_flag(ui->_panelWifi, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui->_panelSettings, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_color(ui->_tabBtnWifi, lv_color_hex(ui->theme->sky_blue), 0);
        lv_obj_set_style_bg_color(ui->_tabBtnSettings, lv_color_hex(ui->theme->dim), 0);
    } else {
        lv_obj_add_flag(ui->_panelWifi, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui->_panelSettings, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_color(ui->_tabBtnWifi, lv_color_hex(ui->theme->dim), 0);
        lv_obj_set_style_bg_color(ui->_tabBtnSettings, lv_color_hex(ui->theme->sky_blue), 0);
    }
}

void UI::_onBackBtnCb(lv_event_t* e) {
    UI* ui = (UI*)lv_event_get_user_data(e);
    ui->showScreen(Screen::DASHBOARD);
}

void UI::_onThemeBtnCb(lv_event_t* e) {
    UI* ui = (UI*)lv_event_get_user_data(e);
    ui->_darkTheme = !ui->_darkTheme;
    UI::theme = ui->_darkTheme ? &DARK : &LIGHT;
    ui->_savePrefs();

    if (ui->_scrDashboard) lv_obj_del(ui->_scrDashboard);
    ui->_buildDashboard();
    if (ui->_scrConfig) lv_obj_del(ui->_scrConfig);
    ui->_buildConfig();

    lv_obj_add_flag(ui->_panelWifi, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui->_panelSettings, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(ui->_tabBtnWifi, lv_color_hex(UI::theme->dim), 0);
    lv_obj_set_style_bg_color(ui->_tabBtnSettings, lv_color_hex(UI::theme->sky_blue), 0);
    lv_scr_load(ui->_scrConfig);
}

void UI::_onSecondsSwitchCb(lv_event_t* e) {
    UI* ui = (UI*)lv_event_get_user_data(e);
    ui->_showSeconds = lv_obj_has_state(ui->_swSeconds, LV_STATE_CHECKED);
    ui->_savePrefs();
}

void UI::_onDateFmtDropdownCb(lv_event_t* e) {
    UI* ui = (UI*)lv_event_get_user_data(e);
    ui->_dateFmt = (lv_dropdown_get_selected(ui->_ddDateFmt) == 0) ? DateFormat::TEXT : DateFormat::NUMERIC;
    ui->_savePrefs();
}

void UI::_onConfigSaveCb(lv_event_t* e) {
    UI* ui = (UI*)lv_event_get_user_data(e);
    if (ui->_onConfigSubmit) {
        ui->_onConfigSubmit(
            lv_textarea_get_text(ui->_taSSID),
            lv_textarea_get_text(ui->_taPass),
            lv_textarea_get_text(ui->_taNTP)
        );
    }
    ui->showScreen(Screen::DASHBOARD);
}

void UI::_onForceSyncCb(lv_event_t* e) {
    UI* ui = (UI*)lv_event_get_user_data(e);
    if (ui->_onForceSync) ui->_onForceSync();
}

void UI::_onTaEvent(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* ta = lv_event_get_target(e);
    lv_obj_t* kb = (lv_obj_t*)lv_event_get_user_data(e);
    if (!kb) return;
    
    if (code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
    } else if (code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(kb, nullptr);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}