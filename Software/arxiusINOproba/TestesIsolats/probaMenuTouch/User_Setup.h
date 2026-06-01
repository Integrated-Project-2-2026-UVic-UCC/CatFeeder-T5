//                            USER DEFINED SETTINGS
//   Set driver type, fonts to be loaded, pins used and SPI control method etc.
//
//   See the User_Setup_Select.h file if you wish to be able to define multiple
//   setups and then easily select which setup file is used by the compiler.
//
//   If this file is edited correctly then all the library example sketches
//   should run without the need to make any more changes for a particular
//   hardware setup! Note that some sketches are designed for a particular TFT
//   pixel width/height

#define USER_SETUP_INFO "ESP32 + ILI9341_DRIVER"

#define ILI9341_DRIVER

#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS 5
#define TFT_DC 22
#define TFT_RST 4

// #define TFT_BL   32
#define TFT_BACKLIGHT_ON HIGH

#define TOUCH_CS 21

#define SPI_FREQUENCY 27000000
#define SPI_READ_FREQUENCY 20000000
#define SPI_TOUCH_FREQUENCY 2500000

// --- Fonts a carregar (ocupen memòria FLASH, les mantenim totes per la UI) ---
#define LOAD_GLCD // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in
                  // FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in
                    // FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in
                    // FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH,
                    // only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in
                    // FLASH, only characters 1234567890:-.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH,
                    // only characters 1234567890:-.
#define SMOOTH_FONT // Utilització de tipografies suaus (bàsic si vols carregar
                    // fitxers vlw futurs)
