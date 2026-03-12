---

# 📊 HX711 + Load Cell Integration

🔗 **Reference Tutorial:**  
https://www.hackster.io/mcmchris/diy-arduino-smart-digital-scale-esp8266-hx711-910945

This section documents the integration of the **HX711 24-bit ADC** with a **load cell** for precise weight measurement in the automatic cat feeder.

---

### 🛠 Hardware Connection

| Load Cell Wire | HX711 Pin | Notes |
|----------------|------------|-------|
| Red (VCC)      | VCC        | Excitation + |
| Black (GND)    | GND        | Excitation − |
| White (SIG−)   | A−         | Signal negative |
| Green (SIG+)   | A+         | Signal positive |

> **Tip:** Use short, twisted pairs for the load cell wiring to minimize noise.

---

### 📌 HX711 Pinout to Microcontroller (ESP32 / Arduino)

| HX711 Pin | MCU Pin | Function |
|-----------|---------|----------|
| VCC       | 3.3V    | Power (use regulator) |
| GND       | GND     | Ground |
| DT        | GPIO XX | Data output |
| SCK       | GPIO YY | Serial clock |

> **Important:** Use the same ground for HX711 and microcontroller.

---

### 📐 Load Cell Calibration

To convert raw ADC counts into a weight:

1. Read raw HX711 value:
---
long reading = scale.read();

# 🛠 Stepper Motors with Arduino + A4988 / DRV8825 Drivers

🔗 **Reference Tutorial:**  
https://www.luisllamas.es/motores-paso-paso-arduino-driver-a4988-drv8825/

This section documents how to interface stepper motors with the Arduino (or ESP32) using **A4988** or **DRV8825** stepper motor drivers.

---

## 🎯 Pin Connections

### 📍 A4988 / DRV8825 to Microcontroller

| Driver Pin | Microcontroller | Function |
|------------|----------------|----------|
| **VMOT**   | External Motor Power | Motor voltage supply |
| **GND**    | GND            | Common ground |
| **VDD**    | 5V (logic)     | Logic power |
| **STEP**   | GPIO (e.g., 2) | Step signal |
| **DIR**    | GPIO (e.g., 3) | Direction control |
| **EN**     | GPIO / Pull-down | Enable driver |
| **MS1, MS2, MS3** | GPIO | Microstep configuration |

> **Note:** DRV8825 supports higher current and more microstepping options than A4988.

---
## 💻 Example Arduino Code

```cpp
#define STEP_PIN 2
#define DIR_PIN  3
#define EN_PIN   4

void setup() {
pinMode(STEP_PIN, OUTPUT);
pinMode(DIR_PIN, OUTPUT);
pinMode(EN_PIN, OUTPUT);

digitalWrite(EN_PIN, LOW); // Enable driver
digitalWrite(DIR_PIN, HIGH); // Set direction
}

void loop() {
digitalWrite(STEP_PIN, HIGH);
delayMicroseconds(800);
digitalWrite(STEP_PIN, LOW);
delayMicroseconds(800);
}

   
   

