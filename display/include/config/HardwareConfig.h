/* ==================== HardwareConfig.h ==================== */
/* AmbiSense Display hardware pin definitions and touchscreen configuration */
#pragma once

/* =============== DISPLAY =============== */
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define BUF_HEIGHT    160

/* =============== TOUCH =============== */
#define TOUCH_CS   33
#define TOUCH_IRQ  36
#define TOUCH_MOSI 32
#define TOUCH_MISO 39
#define TOUCH_CLK  25

/* =============== TOUCH CALIBRATION =============== */
constexpr uint16_t TOUCH_MIN_X = 265;
constexpr uint16_t TOUCH_MAX_X = 4000;
constexpr uint16_t TOUCH_MIN_Y = 100;
constexpr uint16_t TOUCH_MAX_Y = 3950;