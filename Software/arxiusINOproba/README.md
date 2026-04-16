# CatFeeder — ESP32 firmware (base)

Integrated Project II — GR15 [GEMEC-09UV] · Universitat de Vic — UCC

This is the hardware-side Arduino firmware that orchestrates every physical
subsystem of the automatic cat feeder:

| Subsystem         | Component                    | File            |
|-------------------|------------------------------|-----------------|
| MCU               | ESP32 WROOM-32               | `CatFeeder.ino` |
| Display (output)  | 3.5" ILI9488 SPI TFT         | `Display.ino`   |
| Dispenser         | NEMA 17 + DRV8825 / A4988    | `Motor.ino`     |
| Weighing          | 1 kg load cell + HX711       | `Scale.ino`     |
| Ambient sensor    | DHT22                        | `Environment.ino` |
| Real-time clock   | DS3231 (I²C)                 | `RTCManager.ino`|
| User inputs       | 3 push buttons + Squishy     | `Inputs.ino`    |
| Backend           | Supabase REST (WiFi)         | `Network.ino`   |

The touch panel of the ILI9488 is **not** used in this base code: the display
is purely an information board.

---

## 1. Arduino IDE setup

1. Install the **ESP32 board package** (Arduino IDE → *Preferences → Additional
   boards manager URLs*: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`),
   then pick **ESP32 Dev Module** in *Tools → Board*.
2. Install these libraries from *Library Manager* (all available there):

   | Library                          | Author                |
   |----------------------------------|-----------------------|
   | TFT_eSPI                         | Bodmer                |
   | HX711                            | Bogdan Necula (bogde) |
   | DHT sensor library               | Adafruit              |
   | Adafruit Unified Sensor          | Adafruit              |
   | RTClib                           | Adafruit              |
   | AccelStepper                     | Mike McCauley         |
   | ArduinoJson (v7.x)               | Benoît Blanchon       |

3. **Configure TFT_eSPI for the ILI9488**. Open the `User_Setup.h` shipped
   with the library (or enable your own in `User_Setup_Select.h`) and set:

   ```cpp
   #define ILI9488_DRIVER
   #define TFT_MISO 19
   #define TFT_MOSI 23
   #define TFT_SCLK 18
   #define TFT_CS    5
   #define TFT_DC    2
   #define TFT_RST   4
   #define SPI_FREQUENCY       40000000
   #define SPI_READ_FREQUENCY  20000000
   #define LOAD_GLCD
   #define LOAD_FONT2
   #define LOAD_FONT4
   #define LOAD_FONT6
   #define LOAD_FONT7
   #define LOAD_FONT8
   #define SMOOTH_FONT
   ```

4. Open the `CatFeeder` folder in the Arduino IDE — every `.ino` tab and
   `config.h` will appear together.
5. Edit `config.h`:
   - `WIFI_SSID` / `WIFI_PASSWORD`
   - `SUPABASE_URL` / `SUPABASE_ANON_KEY` / `DEVICE_ID`
   - `HX711_CALIBRATION_FACTOR` (see calibration section)
6. Flash at **115200 baud**, Flash Size 4 MB, Partition Scheme "Default 4 MB
   with spiffs".

---

## 2. Wiring (ESP32 WROOM-32)

```
ILI9488 SPI display (VSPI)
  VCC ─ 3V3                  LED ─ 3V3  (backlight always on)
  GND ─ GND                  SDO(MISO) ─ GPIO19
  CS  ─ GPIO5                SDI(MOSI) ─ GPIO23
  RESET ─ GPIO4              SCK       ─ GPIO18
  DC/RS ─ GPIO2              T_* (touch) — leave disconnected

DS3231 RTC                   DHT22
  VCC ─ 3V3                    VCC ─ 3V3
  GND ─ GND                    DATA ─ GPIO27  (10 kΩ pull-up to 3V3)
  SDA ─ GPIO21                 GND ─ GND
  SCL ─ GPIO22

HX711 + load cell            DRV8825 / A4988 + NEMA 17
  VCC ─ 5V                     VMOT  ─ 12 V (motor PSU) + 100 µF cap
  GND ─ GND                    GND   ─ PSU GND (common with ESP32 GND)
  DT  ─ GPIO16                 VDD   ─ 3V3   GND ─ GND
  SCK ─ GPIO17                 STEP  ─ GPIO25
                                DIR   ─ GPIO26
                                EN    ─ GPIO33  (active LOW)
                                MS1/MS2/MS3 set for 1/16 microstepping
                                RESET ─ SLEEP (tied high)
                                A1/A2/B1/B2 → NEMA 17 coils

Buttons (all to GND, internal pull-ups)
  FEED ─ GPIO32
  TARE ─ GPIO13
  MENU ─ GPIO14

Squishy Circuits switch
  one terminal ─ GPIO15, other ─ GND  (internal pull-up)
```

Power comes from the SAI Mini UPS 6000 mAh (5 V). The DRV8825 has its own
motor-rail input (typically 12 V) referenced to the common GND.

---

## 3. Scale calibration

With the tray **empty**:
1. Open the Serial monitor at 115200 baud.
2. From a temporary debug menu (or by calling `scaleCalibrate(100.0)`
   manually) place a known reference weight — e.g. a 100 g standard — on
   the tray.
3. Read the printed calibration factor and paste it into
   `HX711_CALIBRATION_FACTOR` in `config.h`.
4. Re-flash. The firmware will tare automatically on boot.

---

## 4. Cloud protocol

The firmware talks to the **same Supabase backend** documented in the SRS
(`Software/SRS_CatFeeder_WebApp.docx.md` §6). The two actors never talk to
each other directly — the database is the broker:

| Direction                | Supabase table       | Firmware call             |
|--------------------------|----------------------|---------------------------|
| Cloud → device (commands)| `commands`           | `pollPendingCommands()`   |
| Cloud → device (config)  | `device_config`      | `pollDeviceConfig()`      |
| Device → cloud (alive)   | `devices`            | `sendHeartbeat()`         |
| Device → cloud (stream)  | `realtime_weight`    | `pushRealtimeWeight()`    |
| Device → cloud (log)     | `feed_events`        | `logFeedEvent()`          |
| Device → cloud (ack)     | `commands` PATCH     | `updateCommandStatus()`   |

Row Level Security on Supabase must restrict writes so the device can only
INSERT/UPDATE rows matching its own `DEVICE_ID` (SRS §6.5).

---

## 5. Next steps

- [ ] Replace `setInsecure()` with the pinned Supabase CA bundle.
- [ ] Add a debug Serial menu: tare, calibrate, reset error, manual feed.
- [ ] Persist schedules and calibration in NVS (so a cold boot without
      internet still honours the last known config — SRS §REQ-NF-10).
- [ ] Wire OTA updates (ArduinoOTA + Supabase Storage bucket).
- [ ] Add motor-current sensing (DRV8825 refs) to detect jams earlier.
