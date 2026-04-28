// ==========================================================================
// User_Setup.h - Configuració per TFT_eSPI (CatFeeder Project)
// Substitueix aquest fitxer pel teu "User_Setup.h" original a la carpeta:
// Documents/Arduino/libraries/TFT_eSPI/
// ==========================================================================

#define USER_SETUP_INFO "User_Setup"

// --- Driver de la Pantalla ---
#define ILI9488_DRIVER

// --- Configuració de Pins ESP32 ---
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   15  // Chip select control pin
#define TFT_DC    2  // Data Command control pin
#define TFT_RST   4  // Reset pin (could connect to RST pin)

// --- Configuració de Pins Tàctil (XPT2046) ---
#define TOUCH_CS            5  // Chip select pin (T_CS) per al controlador tàctil

// --- Fonts a carregar (ocupen memòria FLASH, les mantenim totes per la UI) ---
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define SMOOTH_FONT // Utilització de tipografies suaus (bàsic si vols carregar fitxers vlw futurs)

// --- Configuració de Freqüències SPI ---
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000  // 2.5 MHz recomanat per estabilitat amb el touch XPT2046
