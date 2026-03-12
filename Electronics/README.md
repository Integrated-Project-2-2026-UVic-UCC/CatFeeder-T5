
# ⚡ Electronics System  
**Automatic Cat Feeder – Integrated Project II (GR15 [GEMEC-09UV])**  
Universitat de Vic – Universitat Central de Catalunya  
Academic Year 2025–2026  

---

## 📌 Overview

This folder contains the complete design and documentation of the **electronic subsystem** of the automatic cat feeder.

The electronics system is responsible for:

- Precise motor control  
- Accurate weight measurement  
- Reliable time-based operation  
- Safe power distribution  
- Hardware-level protection mechanisms  

The design prioritizes **robustness, electrical safety, noise immunity, and modular expandability**, ensuring reliable long-term operation under real-world conditions.

---

## 🎯 Electronics Objectives

- Provide stable and regulated power to all subsystems  
- Control the dispensing motor safely and efficiently  
- Measure dispensed food weight using a load cell system  
- Interface reliably with the microcontroller (ESP32 / Arduino-based platform)  
- Prevent electrical faults (overcurrent, reverse polarity, voltage spikes)  
- Minimize electromagnetic interference (EMI)  
- Ensure maintainability and future scalability  

---

## 🧩 System Architecture

The electronics subsystem is composed of the following modules:

### 1️⃣ Control Unit
- Microcontroller (ESP32 / Arduino platform)  
- GPIO control for motor driver  
- HX711 communication (load cell ADC)  
- Scheduling interface with firmware  

### 2️⃣ Motor Control Stage
- DC motor driver 
- Flyback protection  
- Current limitation strategy  
- Thermal considerations  

### 3️⃣ Weight Measurement System
- Load cell (strain gauge bridge)  
- HX711 24-bit ADC  
- Hardware noise filtering  
- Stable reference and grounding strategy  

### 4️⃣ Power Management
- Input supply stage  
- Voltage regulation (buck / linear regulators)  
- Reverse polarity protection  
- Decoupling and filtering  
- Star-ground topology  

---

## 📂 Folder Structure
electronics/

- schematics/          → Circuit schematics (PDF / CAD files)
- pcb/                 → PCB layout files (if applicable)
- datasheets/          → Relevant component datasheets
- calculations/        → Design calculations (power, current, filtering)
- testing/             → Validation and measurement reports
- README.md            → This document


---

## ⚙️ Key Components

| Component        | Function                          |
|------------------|-----------------------------------|
| ESP32 / Arduino  | Main control unit                 |
| HX711            | 24-bit ADC for load cell          |
| Load Cell        | Food weight measurement           |
| Motor Driver     | DC motor control                  |
| Voltage Regulator| Stable power supply               |

---


## 📚 References

- ESP32 Technical Reference Manual  
- HX711 Datasheet  
- Load Cell Application Notes  
- TI / ST Microelectronics Power Design Guidelines  
- IPC-2221 PCB Design Standard  

(Complete references available in Milestone documentation)
