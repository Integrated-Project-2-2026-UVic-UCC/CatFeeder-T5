# 2. Objectives

This section defines what the CatFeeder-T5 is expected to achieve by the end of the project. The objectives were written after a review of existing feeders and a concrete analysis of what is technically feasible within the scope of Integrated Project II at UVic-UCC.

---

## 2.1 Main Objective

Design and build a gravimetric automatic cat feeder controlled by an ESP32 microcontroller, capable of dispensing precise dry food portions on a schedule defined by the user — locally or remotely — while logging every feeding event to a cloud database and providing both a physical touchscreen interface and a web dashboard for monitoring and control.

---

## 2.2 Specific Objectives

### SO1 — Gravimetric portion control
Dispense food portions with an error below ±5 g per cycle. The dispensing mechanism (NEMA 17 stepper motor via DRV8825) operates in a closed loop with the HX711 load cell: the motor runs until the measured weight reaches the target value, then stops. Volume-based estimation is explicitly avoided.

### SO2 — Schedule-based autonomous operation
Execute feeding schedules without requiring any user action at the time of feeding. Schedules are stored both in the DS3231 RTC (for offline operation during network outages) and in Supabase (as the primary remote configuration source). If WiFi is unavailable, the RTC-backed schedule continues to run independently.

### SO3 — Remote configuration and monitoring
Allow users to define and modify feeding schedules, portion sizes, and device settings from a web dashboard (React/Vite) without physical access to the device. The ESP32 polls Supabase via REST at regular intervals to fetch new commands and push telemetry data (weight, temperature, humidity, feed events).

### SO4 — Physical touchscreen interface
Provide a standalone UI on a 3.5" ILI9488 touchscreen that allows manual food dispensing, mode switching, and basic status reading without any network connection. The interface includes an idle screensaver and a four-corner unlock pattern to prevent accidental activation by the cat.

### SO5 — Environmental and operational telemetry
Record and transmit temperature (DHT22) and humidity readings alongside each feed event. This data is stored in Supabase and accessible from the dashboard, providing context for detecting anomalies in food storage conditions.

### SO6 — AI-assisted feeding advisor
Integrate an n8n webhook-based chatbot ("VetGat") that queries the Supabase feeding history and provides personalised nutrition and health observations. The chatbot should detect patterns such as missed meals, consistent under-eating, or critically low hopper weight, and report them proactively.

### SO7 — Modular and non-blocking firmware architecture
Implement the ESP32 firmware as a set of independent modules (motor, scale, RTC, network, display, sensors) coordinated by a finite state machine. The main loop must never block: all timing uses `millis()`-based intervals, and `AccelStepper::run()` is called on every iteration to maintain smooth motor motion regardless of network or sensor activity.

### SO8 — Functional validation
Test each subsystem individually before integration, and validate the complete system against three core metrics: portion accuracy (target: <±5 g), schedule execution reliability (target: 100% of scheduled feeds executed within ±30 s), and system uptime over a 48-hour continuous run.

---

## 2.3 Out of Scope

The following items were identified and deliberately excluded from the project scope:

- Visual food-level detection via camera (reserved as a future extension).
- Multi-cat identification (a single feeding profile is assumed per device).
- Battery-backed UPS for extended offline autonomy (the RTC fallback covers short outages; a full UPS is not implemented).
- Native mobile app (the web dashboard is responsive and covers this use case).

