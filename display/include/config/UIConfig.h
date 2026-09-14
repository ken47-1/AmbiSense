/* ==================== UIConfig.h ==================== */
/* AmbiSense Display UI configuration */
#pragma once

/* =============== SCREEN LAYOUT =============== */
constexpr int SCR_W = 320;
constexpr int SCR_H = 240;
constexpr int MARGIN = 10;
constexpr int DIV_MARGIN = 5;

/* =============== STRINGS =============== */
static const char* MONTH_NAMES[] = {
    "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char* DAY_NAMES[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};

/* =============== CLOCK =============== */
constexpr int CLK_SIZE = 140;
constexpr int CLK_R = CLK_SIZE / 2;
constexpr int CLK_CX = CLK_SIZE / 2;
constexpr int CLK_CY = CLK_SIZE / 2;

/* =============== ROW OFFSETS (empirical centering) =============== */
constexpr int R1_X_OFFSET = -2;
constexpr int R2_X_OFFSET = -3;
constexpr int R3_X_OFFSET = -3;
constexpr int R4_X_OFFSET = -4;
constexpr int R5_X_OFFSET = -4;
constexpr int R6_X_OFFSET = -3;
constexpr int R7_X_OFFSET = -3;
constexpr int R8_X_OFFSET = -4;

/* =============== ROW GAPS (empirical spacing) =============== */
constexpr int R1_ICON_GAP  = 5;
constexpr int R3_ICON_GAP  = 1;
constexpr int R5_HUMI_GAP  = 0;
constexpr int R5_PRESS_GAP = 4;
constexpr int R6_ICON_GAP  = 4;
constexpr int R7_ICON_GAP  = 4;
constexpr int R8_TEMP_GAP  = 0;
constexpr int R8_HUMI_GAP  = 0;

constexpr int R5_PAIR_GAP = 10;
constexpr int R7_PAIR_GAP = 10;
constexpr int R8_PAIR_GAP = 11;

/* ============ DERIVED LAYOUT CONSTANTS ============ */
// Computed from SCR_W, SCR_H, CLK_SIZE, MARGIN
constexpr int CLK_X  = (SCR_W - CLK_SIZE - MARGIN + (CLK_SIZE/2 - CLK_R));
constexpr int CLK_Y  = MARGIN;
constexpr int VDIV_X = (CLK_X - MARGIN - 2);

/* =============== CONFIG SCREEN =============== */
constexpr int TAB_H = 36;
constexpr int KB_H = 120;
constexpr int BTN_H = 36;