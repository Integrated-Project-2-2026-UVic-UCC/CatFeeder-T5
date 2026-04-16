# Software and Hardware Orchestration: Next Steps

This document outlines the steps to successfully orchestrate all peripherals using the Arduino code provided in the `arxiusINOproba` directory.

Since we are prioritizing a **physical button panel (botonera)** over the touch screen, touch capabilities have been disabled and their pins freed up. Below is the master pinout map tailored to prevent conflicts, followed by the integration steps.

## 1. Unified Pinout Map

Based on `pins.md` and the existing `config.h`, some default pins conflicted (e.g., CS and Squishy both using 15, or DIR using a DHT pin). Here is the updated, conflict-free pinout you must implement in your hardware and `config.h`:

| Peripheral | Component | ESP32 GPIO | Notes |
| :--- | :--- | :--- | :--- |
| **Display (TFT)** | MOSI / SDI | **23** | SPI Data In |
| | MISO / SDO | **19** | SPI Data Out |
| | SCLK / SCK | **18** | SPI Clock |
| | CS | **15** | Chip Select |
| | DC | **2** | Data / Command |
| | RESET | **4** | Reset |
| | LED (Backlight) | **32** | Display backlight |
| | *Touch Pins* | *None* | *Not used (T_CS, T_CLK, T_DIN, T_DO omitted)* |
| **Stepper Motor** | EN | **25** | Enable (Active Low) |
| *(DVR8825)* | STEP | **26** | Step |
| | DIR | **27** | Direction |
| **Load Cell** | DT | **16** | Data |
| *(HX711)* | SCK | **17** | Clock |
| **Temperature** | DHT22 Pin | **33** | Allocated to an empty pin |
| **RTC (I2C)** | SDA | **21** | Standard I2C line |
| *(DS3231)* | SCL | **22** | Standard I2C line |
| **Botonera** | BTN_FEED | **13** | Manual dispense button |
| *(Control Panel)*| BTN_TARE | **14** | Tare scale button |
| | BTN_MENU | **34** | Cycle display screens |
| | SQUISHY_PIN | **35** | Conductive dough / cat presence contact |

---

## 2. Step-by-Step Hardware & Software Integration

### Step 1: Update `config.h` variables
Before compiling the code, open `arxiusINOproba/config.h` and update the constants to match the table exactly. For example:
```cpp
// Display Configuration (Make sure TFT_eSPI User_Setup.h matches these too)
#define DHT_PIN                 33
#define STEPPER_STEP            26
#define STEPPER_DIR             27
#define STEPPER_EN              25
#define BTN_FEED                13
#define BTN_TARE                14
#define BTN_MENU                34
#define SQUISHY_PIN             35
```

### Step 2: Configure `TFT_eSPI` Library (Display)
For the display to work with the script in `Display.ino`, you must go to your Arduino libraries folder (`Documents/Arduino/libraries/TFT_eSPI/`), open `User_Setup.h`, and configure the exact SPI pins: `TFT_CS (15)`, `TFT_DC (2)`, `TFT_MOSI (23)`, `TFT_SCLK (18)`, `TFT_RST (4)`. Ensure you select the correct driver (e.g., ILI9488) in that file.

### Step 3: Wire the Stepper Motor Driver (DVR8825)
1. Wire `EN` (25), `STEP` (26), and `DIR` (27) from the ESP32 to the driver.
2. Provide a **separate 12V external power supply** safely to the `VMOT` and `GND` pins on the driver. *Do not power the motor from the ESP32's 5V or 3V3 pins!*
3. Connect the stepper coils (A1, A2, B1, B2) to the motor directly.

### Step 4: Build the Physical Botonera
Since we aren't using the touch screen, wiring the hardware buttons is critical. The `Inputs.ino` logic relies on internal pull-up resistors (`INPUT_PULLUP`). 
1. Wire one prong of each button directly to its respective GPIO (`13`, `14`, `34`, `35`). (Note: 34 and 35 don't have internal pullups, so add external 10k pull-up resistors to 3V3 for those, or move them to GPIO 12 and 0 if you don't want to use external resistors).
2. Wire the opposite prong of each button to **GND**.

### Step 5: Wire I2C and Analog Sensors (RTC, Scale, DHT)
1. Connect the HX711 and DS3231 to the 3.3V or 5V rails, making sure I2C pins (`21`, `22`) go to the RTC, and (`16`, `17`) to the HX711 amp.
2. Wire the DHT22 to GPIO `33` with a 10K pull-up resistor from data to VCC if it's not a pre-assembled module.

### Step 6: Connect to Supabase
In `config.h`, fill in your precise `SUPABASE_URL` and `SUPABASE_ANON_KEY`, along with a unique `DEVICE_ID`. 

### Step 7: Calibration and Compilation
1. Connect the ESP32 via USB.
2. Open `CatFeeder.ino` in the Arduino IDE. 
3. Install required libraries (`ArduinoJson`, `TFT_eSPI`, `HX711`, `DHT`, `RTClib`, `AccelStepper`).
4. Flash the code to the ESP32.
5. Watch the **Serial Monitor (115200 baud)**. Upon startup, it should initialize network, display, sensors, and scale. If the scale weight is incorrect, apply a known weight to the tray, obtain the raw values, update the `HX711_CALIBRATION_FACTOR` in `config.h`, and re-flash.
