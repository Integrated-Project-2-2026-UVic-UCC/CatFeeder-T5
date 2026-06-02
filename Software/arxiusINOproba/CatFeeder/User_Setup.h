//                            USER DEFINED SETTINGS
// ==========================================================================
// CatFeeder T5 — TFT_eSPI configuration for ESP32 + ILI9341 2.8" 320x240
// with XPT2046 touch controller sharing the same SPI (VSPI) bus.
//
// NOTE: TFT_eSPI normally reads its own User_Setup.h inside the library
// folder. To make this file the active one, either:
//   a) copy it over <Arduino/libraries/TFT_eSPI/User_Setup.h>, or
//   b) compile with -DUSER_SETUP_LOADED and -include this file (build flag).
// ==========================================================================

#define USER_SETUP_INFO "CatFeeder ESP32 + ILI9341_DRIVER"

#define ILI9341_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   22
#define TFT_RST  4

// No backlight pin — BL is wired directly to VCC.
#define TFT_BACKLIGHT_ON HIGH

#define TOUCH_CS 21

// --- Fonts (kept for the later display phase) ------------------------------
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define SMOOTH_FONT

// --- SPI clocks -----------------------------------------------------------
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY   6000000
#define SPI_TOUCH_FREQUENCY  2500000
