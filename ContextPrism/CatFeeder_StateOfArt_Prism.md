# Introduction / State of the Art — Benchmarking

---

## Background and Motivation

Pet ownership in Spain has grown substantially over the past decade. According to sector data published by ANFAAC, the country currently has more than 20 million registered companion animals [7]. A significant portion of these are domestic cats, and their owners increasingly face the same scheduling pressures that make consistent daily care difficult: long working hours, irregular routines, and short absences from home.

Feeding is one of the most critical routines in cat care — not because it is technically complex, but because it must happen at consistent times and in accurate quantities. Irregular meal timing causes anticipatory anxiety and food-seeking behaviour in cats [49]. Chronic overfeeding contributes to obesity, which is directly associated with diabetes mellitus, hepatic lipidosis, osteoarthritis, and feline lower urinary tract disease (FLUTD) [2]. The World Small Animal Veterinary Association (WSAVA) and the American Animal Hospital Association (AAHA), together with the American Association of Feline Practitioners (AAFP), explicitly discourage the practice of free-feeding (leaving food available at all times) and instead recommend portion control based on caloric calculations tied to each animal's body condition score and metabolic rate [50] [52].

The practical problem is straightforward: most cat owners cannot be home for every meal, every day. Automatic feeders exist precisely to fill this gap. The engineering problem, however, is that most available solutions — both commercial and academic — fail to meet basic requirements for portion accuracy and reliable remote control. This project proposes a prototype that addresses these failures by combining closed-loop gravimetric dispensing, offline scheduling redundancy, a cloud backend, and an AI-assisted health monitoring layer.

---

## Why This Work Is Interesting

The project is worth doing for two reasons that do not overlap much.

The first is practical. A feeder that dispenses the wrong amount of food — or stops working when the Wi-Fi drops — is not just inconvenient; it can directly harm the animal it is supposed to care for. Getting this right requires solving a set of interconnected engineering problems: storing dry kibble without jamming, dispensing it in gram-accurate quantities regardless of kibble geometry, maintaining scheduled operation during network outages, and giving the owner meaningful remote visibility. None of these problems is particularly hard in isolation. Combined in a single low-cost embedded device, they require careful architectural choices.

The second reason is technical scope. The project integrates embedded firmware, motor control, sensor calibration, cloud connectivity, a reactive web dashboard, and a conversational AI agent. This breadth is unusual for a single prototype. The choices made at each layer — from the stepper motor driver to the database schema — interact with each other, and getting those interactions right is the core engineering challenge.

---

## Previous Works and Their Limitations

### 1. Automatic and Smart Pet Feeders: Research Prototypes and Commercial Products

Academic work on automatic pet feeders traces a clear progression. Early prototypes used 8-bit microcontrollers — PIC, Arduino Uno, Arduino Mega 2560 — with a DS3231 real-time clock module to trigger a small servo motor at scheduled intervals [1]. The servo either rotated a paddle or opened a trapdoor for a fixed duration. Some researchers added an HC-SR04 ultrasonic sensor to estimate food levels in the hopper, and a few explored solar-powered variants to address grid dependency [1] [2].

The fundamental limitation of these early designs is that they all use open-loop volumetric dispensing: the microcontroller runs the motor for a set time and assumes a consistent volume — and therefore a consistent weight — is released. This assumption fails in practice. Dry kibble settles in the hopper over time, changing its bulk density. Different kibble shapes and sizes flow differently. A rotation that releases 20 g one morning may release 14 g the next [12]. No academic prototype in this generation included any feedback mechanism to verify the dispensed quantity.

As Wi-Fi-capable SoCs became widely available, the ESP8266 and later the ESP32 replaced earlier microcontrollers in research prototypes, enabling local web interfaces and basic smartphone integration [4] [5]. Some of these works reported improved scheduling flexibility, but the dispensing mechanism remained volumetric in nearly all cases [3].

On the commercial side, the market is dominated by products from PETKIT, PetSafe, SureFeed, Xiaomi, and Petlibro. These devices offer polished mobile applications, push notifications, and in some cases camera monitoring. The table below summarises their key characteristics.

| Brand & Model | Dispensing | Connectivity | Notable Feature | Key Limitation |
|:---|:---|:---|:---|:---|
| Petlibro Granary AF203 | Volumetric (silicone rotor) | Wi-Fi, proprietary app | Built-in camera, infrared level sensor | No load cell; up to ±4 g error on a 10 g portion (40% margin) [12] |
| PETKIT Fresh Element | Volumetric / mixed | Wi-Fi, proprietary app | Dual-hopper for food mixing | Closed ecosystem; no third-party smart home integration [8] |
| SureFeed Microchip | Motorised lid | RFID / Wi-Fi hub | Per-cat access control via microchip | Does not dispense from a hopper; only controls bowl access [15] |
| PetSafe Smart Feed | Volumetric (conveyor belt) | Wi-Fi, proprietary app | Slow-feed mode over 15 minutes | Subscription fees for advanced features; jams with large kibble [8] |
| Xiaomi Smart Feeder | Volumetric (rotor) | Wi-Fi (Mi Home) | Smart home integration | Closed API; dependent on regional cloud servers [8] |

The Petlibro AF203 is particularly instructive. It is one of the most reviewed smart feeders on the market, and its manufacturer explicitly acknowledges in its support documentation that portion weight can vary by up to 4 g on a standard 10 g serving [12]. For a cat on a strict dietary protocol, this is a meaningful error. The absence of a load cell in any of these products is not an oversight — it is a cost decision — but it means that strict caloric management is effectively impossible with any current commercial feeder.

Beyond portion accuracy, commercial feeders operate within proprietary ecosystems. The cloud servers that power their mobile apps may be discontinued. Their APIs are closed, which prevents integration with open platforms like Home Assistant or custom automation scripts. When a cloud subscription lapses or a server is retired, the device often loses most of its functionality.

### 2. Gravimetric Dispensing with Load Cells and the HX711

The solution to open-loop volumetric inaccuracy is a closed-loop gravimetric feedback system. In embedded applications, this means a strain gauge load cell connected to an HX711 24-bit ADC.

A single-point load cell works by converting mechanical deformation into a resistance change. When food lands in the bowl, the aluminium beam of the load cell deflects. Four strain gauges bonded to its flexure points form a Wheatstone bridge; their combined resistance change produces a small differential voltage (typically 1–2 mV per volt of excitation) that the HX711 amplifies and converts to a 24-bit digital value at either 10 Hz or 80 Hz [7] [16].

In practice, this signal is noisy. Stepper motor coils generate electromagnetic interference. Hard kibble impacting a metal bowl produces sharp mechanical transients. Long-term sensor creep slowly shifts the zero-point offset. Reliable gravimetric measurement in a mechatronic environment requires both calibration and filtering [6].

Calibration maps raw ADC counts to known reference weights using a linear regression that yields a scale factor and a zero offset. Auto-taring — periodically recalculating the offset while the load cell is unloaded — compensates for thermal and structural drift [7].

For filtering, the literature describes several approaches with different tradeoffs:

| Filter Type | Mechanism | Suitable For | Limitation |
|:---|:---|:---|:---|
| Simple Moving Average (SMA) | Averages the last N readings | Low-noise environments | Phase delay; vulnerable to impact spikes [17] |
| Median Filter | Returns the middle value of a sorted window | Impulse noise rejection | Higher RAM and CPU cost than SMA [16] |
| IIR (Infinite Impulse Response) | Weighted sum of current inputs and past outputs | Steady-state resonance suppression | Can produce instability if coefficients are poorly designed [7] |
| Kalman Filter | Recursive prediction-correction based on measurement variance | High-vibration environments | Requires matrix operations; computationally expensive [23] |

For the proposed feeder, the chosen approach is a median filter applied to a rolling window of HX711 samples, combined with hardware capacitors on the ADC input pins for baseline EMI rejection. This provides robust impulse noise rejection without the computational overhead of a full Kalman implementation.

The load cell output drives the dispensing control loop. As the measured weight approaches the target, the NEMA 17 stepper motor — driven by a DRV8825 driver — decelerates to prevent overshoot. The NEMA 17 was selected over servo motors and smaller gear steppers because of its holding torque (40–60 N·cm), which is sufficient to force past kibble jams without stalling [24]. The DRV8825 supports up to 1/32 microstepping, which reduces mechanical resonance and acoustic noise. The AccelStepper library manages non-blocking trapezoidal acceleration profiles so the motor control loop is never interrupted by other firmware tasks [27] [28].

### 3. ESP32-Based IoT Systems for Embedded Animal Care

The ESP32 WROOM-32 was chosen as the microcontroller for several reasons beyond its native Wi-Fi and BLE support. Its dual-core Xtensa LX6 architecture, running at up to 240 MHz, allows motor control and network communication to run on separate cores under FreeRTOS [5].

This separation matters. When an ESP32 negotiates a TLS handshake or parses a large JSON payload from the cloud, that operation can take tens of milliseconds. On a single-core device, or on a dual-core device where both tasks share the same execution thread, this stalls the motor control loop, causing the stepper to miss steps or jitter. Assigning the AccelStepper loop and HX711 polling to Core 1 (APP_CPU) and all network operations to Core 0 (PRO_CPU) eliminates this interference [31] [32].

Beyond task segregation, the firmware avoids blocking delays throughout. The standard `delay()` function in Arduino suspends execution entirely, which breaks network connections, misses hardware interrupts, and prevents watchdog timer refreshes. The literature is consistent on this point: production-grade embedded IoT systems use `millis()`-based scheduling and non-blocking state machines instead [30] [36]. The feeder's firmware transitions through defined states — IDLE, TARRING, DISPENSING, PAUSED, ERROR — using elapsed-time comparisons rather than blocking delays.

For cloud communication, the firmware polls the Supabase REST API over HTTPS using a zero-buffer stream parsing approach. Rather than loading the full HTTP response body into heap memory (which risks fragmentation and crashes on a device with limited RAM), the parser reads the TCP stream byte by byte and extracts target keys directly [40].

Offline reliability is provided by a DS3231 RTC module, which maintains accurate time via its temperature-compensated crystal oscillator and a coin-cell backup battery. If Wi-Fi connectivity is lost, the firmware falls back to DS3231 timestamps and continues executing scheduled feedings autonomously, queuing events locally for cloud synchronisation when connectivity is restored [1].

The 3.5" ILI9488 touchscreen with XPT2046 controller connects over SPI. This combination has a known hardware issue: the ILI9488 does not tri-state its MISO line when its chip select is deasserted, which prevents the ESP32 from communicating with the XPT2046 on the same SPI bus [43] [44]. The firmware resolves this through careful SPI transaction management via the TFT_eSPI library, and by assigning the XPT2046 chip select to GPIO 5 rather than GPIO 21, which would conflict with the DS3231 I2C SDA line.

The DHT22 sensor monitors the humidity and temperature inside the food hopper. If humidity exceeds safe thresholds, stored kibble becomes susceptible to oxidative rancidity and mould growth. AAHA guidelines explicitly address food storage conditions as part of their nutritional safety framework [52].

### 4. Feline Nutrition, Portion Control, and AI-Assisted Dietary Monitoring

Veterinary guidelines from WSAVA and AAHA/AAFP frame portion accuracy as a medical requirement, not just a convenience feature. The standard caloric calculation begins with the Resting Energy Requirement (RER):

$$\text{RER} = 70 \times W_{kg}^{0.75}$$

where $W_{kg}$ is the cat's body weight in kilograms. The Maintenance Energy Requirement (MER) is then calculated by multiplying the RER by a life-stage factor [53] [54] [55]:

| Life Stage / Condition | MER Multiplier | Notes |
|:---|:---|:---|
| Healthy, intact adult | 1.4–1.6 × RER | Baseline for active, non-neutered cats |
| Neutered adult | 1.2 × RER | Neutering reduces metabolic rate; requires caloric reduction |
| Inactive / obesity-prone | 1.0 × RER | For sedentary indoor cats maintaining current weight |
| Weight loss protocol | 0.8 × RER (of ideal weight) | Applied to ideal weight, not current weight [56] |
| Kittens under 4 months | 2.5–3.0 × RER | High requirement for skeletal and neurological growth |

Translating these caloric targets into physical food portions is complicated by the wide variation in commercial kibble caloric density (300–500 kcal/cup depending on brand and formula) [57]. A gravimetric feeder bypasses this ambiguity entirely: it dispenses a specific gram-weight and stops when the target is reached, regardless of how the kibble is packed in the hopper.

Beyond static portion control, recent research has begun applying anomaly detection to feeding behaviour logs. If a cat consistently takes longer to finish its portion, or leaves a larger fraction uneaten over several days, these trends can indicate dental problems, stomatitis, or early renal disease before clinical symptoms appear [10]. The VetGat AI chatbot implemented in this project ingests time-series feeding data from the Supabase backend and uses Retrieval-Augmented Generation (RAG) to ground its dietary recommendations in verified WSAVA and AAHA guidelines, rather than generating responses from the language model alone [58] [69].

### 5. Cloud Backends and Web Dashboards for IoT Devices

Supabase was selected as the cloud backend over alternatives like Firebase because it is built on PostgreSQL, which is well-suited for structured, relational telemetry data [39] [60]. For each database table, Supabase automatically generates a REST API via the PostgREST layer, allowing the ESP32 to execute CRUD operations over HTTPS without any custom middleware [61] [62].

Row-Level Security (RLS) policies, enforced natively within Postgres, ensure that each device can only read its own configuration commands and write to its own telemetry rows [63]. Supabase's Realtime API, powered by an Elixir/Phoenix server, streams database write-ahead-log changes to connected clients via WebSockets. This allows the React/Vite dashboard to update live when the ESP32 posts a new feed event, without the dashboard needing to poll [64].

Edge Functions — serverless scripts running in a Deno runtime — handle asynchronous backend logic such as triggering push notifications when humidity thresholds are exceeded, without requiring the ESP32 to manage notification delivery directly [39].

The web dashboard itself is structured as a React Single Page Application (SPA) built with Vite. It provides six functional sections: dashboard overview, cat profiles, feeding schedules, history and analytics, device management, and calibration settings.

The VetGat chatbot is orchestrated using n8n, a node-based workflow automation tool. The architecture follows the RAG pattern: veterinary documentation is chunked, converted to vector embeddings, and stored in Supabase via the pgvector extension. When a user submits a query, n8n retrieves the most relevant documentation chunks by cosine similarity, queries the live feeding database, and passes both to the language model to generate a grounded, data-specific response [69] [71] [72].

---

## Summary of Gaps and How This Project Addresses Them

The problems are consistent across the literature. Commercial feeders accept large portion errors because adding a load cell increases unit cost. Their cloud ecosystems are proprietary and fragile. Academic prototypes have better architecture transparency but rarely combine accurate sensing, cloud connectivity, and intelligent monitoring in a single coherent system.

This project addresses these gaps directly. Closed-loop gravimetric control with an HX711 and a microstepped NEMA 17 replaces open-loop volumetric dispensing. FreeRTOS dual-core task segregation ensures the motor control loop is never blocked by network operations. DS3231 hardware backup makes the device reliable when Wi-Fi is unavailable. Supabase provides an open, self-hostable backend without proprietary lock-in. The VetGat RAG agent turns feeding telemetry into actionable dietary advice grounded in WSAVA and AAHA clinical guidelines.

---

## References

[1] Y. M. Yatim et al., "Automatic solar-powered pet food dispenser system," *E3S Web of Conferences*, vol. 07032, 2024.

[2] "Automation of pet feeders using the Internet of Things (IoT)," *2021 IEEE International Conference on Mobile Networks and Wireless Communications (ICMNWC)*, Dec. 2021.

[3] "Design and Development of a Smart Pet Feeder with IoT and Deep Learning," *MDPI Engineering Proceedings*, vol. 82, no. 1, 2023.

[4] "Automatic Pet Feeder," *University of Illinois ECE445 Project Report*, 2016.

[5] "IoT-Based Smart Pet Feeding System with Dynamic Network Configuration Using ESP32, WiFiManager and Blynk 2.0," *IIETA*, 2023.

[6] "An automatic weighing device for measuring the consumption of food," *PLOS One*, 2024.

[7] "A Novel and Self-Calibrating Weighing Sensor with Intelligent Closed-Loop Control," *MDPI Electronics*, vol. 13, no. 9, 2024.

[8] "Smart Pet Products Info and Review," *AliExpress Tech Reviews*, 2024.

[10] *Smart Pet Products Market Research Report 2034*, Dataintelo, 2025.

[12] "How to Troubleshoot Inconsistent Food Portion Weights on the Camera Feeder," Petlibro Support, 2024. [Online]. Available: https://petlibro.com/pages/what-should-i-do-if-the-weight-of-each-portion-of-food-fed-by-af203-camera-feeder-is-different

[15] "The SureFeed Microchip Pet Feeder Connect," Sure Petcare. [Online]. Available: https://www.surepetcare.com

[16] "HX711: 24-bit Delta Sigma ADC interface for weight scale," *Infineon Developer Community*, 2024.

[17] "Mastering Precision Weighing: A Deep Dive into the HX711 Module for Arduino Projects," AliExpress Wiki, 2024.

[23] "Sensor fusion-based IoT framework for precision livestock monitoring and feed management," *IJATEE*, vol. 2025, no. 6, 2025.

[24] "How to Use Stepper Motor with Arduino: NEMA 17 + A4988 Guide," Zbotic. [Online]. Available: https://zbotic.in

[27] "Running two stepper motors (DRV8825 Drivers) simultaneously with AccelStepper," *Arduino Forum*, 2020.

[28] "Stepper Motor Acceleration & Deceleration: AccelStepper Library Guide," Zbotic. [Online]. Available: https://zbotic.in

[30] "L7: Internet of Things | Physical Computing," *Makeability Lab, University of Washington*. [Online]. Available: https://makeabilitylab.github.io/physcomp/esp32/iot.html

[31] "Smooth Stepper Motor Control with ESP32 Dual Core," *Protonest IoT on Medium*, 2023.

[32] "Stepper motor dual core tasks," *Reddit / r/esp32*, 2020.

[36] "Arduino vs. Xedge — blocking vs. non-blocking loops," *Real Time Logic*. [Online]. Available: https://realtimelogic.com

[39] A. Venkatesh, "Building Lean IoT Startups with Supabase — Part 2: Designing a Real IoT Backend without the Bloat," *Medium*, Nov. 2025.

[40] P. Fonseca, "How to Call a REST API on ESP32 in One Line of Code, No JSON Parsing, No Memory Leaks," *Medium*, 2023.

[43] "Help with touch for TFT 3.5" ILI9488 with ESP32," *Bodmer TFT_eSPI GitHub Discussions*, #3606, 2025.

[44] "Esp32s3 ILI9488 + XPT2046 Touch Issues," *Bodmer TFT_eSPI GitHub Issues*, #3836, 2026.

[49] C. A. Tony Buffington et al., "AAFP-AAHA: Feline Life Stage Guidelines," *Journal of Feline Medicine and Surgery*, 2010, republished *PMC*, 2024.

[50] "2021 AAHA/AAFP Feline Life Stage Guidelines," *PMC*, 2024. [Online]. Available: https://pmc.ncbi.nlm.nih.gov/articles/PMC10812130/

[52] "Helping Pets Live Healthier, Thinner Lives: AAHA Nutritional Assessment Guidelines," *U.S. Food and Drug Administration*. [Online]. Available: https://www.fda.gov/animal-veterinary

[53] "Nutritional Requirements of Small Animals," *Merck Veterinary Manual*. [Online]. Available: https://www.merckvetmanual.com

[54] "Calculating Calories Based on Pet Needs," *Pet Nutrition Alliance*, 2023. [Online]. Available: https://petnutritionalliance.org

[55] "RER BWkg0.75 70 MER RER life stage factor," *AAHA*. [Online]. Available: https://www.aaha.org

[56] "Weight Reduction in the Obese Pet," *AAHA Nutrition and Weight Management Guidelines*, 2021. [Online]. Available: https://www.aaha.org

[57] "Feeding Plans for Healthy, Appropriate Weight Cats and Dogs," *AAHA*, 2021. [Online]. Available: https://www.aaha.org

[58] "AI Agent integrations," n8n.io. [Online]. Available: https://n8n.io/integrations/agent/

[60] "The Postgres Development Platform," Supabase. [Online]. Available: https://supabase.com

[61] "Architecture," *Supabase Docs*. [Online]. Available: https://supabase.com/docs/guides/getting-started/architecture

[62] "Data REST API," *Supabase Docs*. [Online]. Available: https://supabase.com/docs/guides/api

[63] "Supabase vs Auth0," *Supabase Docs*. [Online]. Available: https://supabase.com/alternatives/supabase-vs-auth0

[64] "Realtime," *Supabase Docs*. [Online]. Available: https://supabase.com/docs/guides/realtime

[69] "Build Custom RAG Systems With Logic & Control," n8n.io. [Online]. Available: https://n8n.io/rag/

[71] "The NEW Way to Build RAG Agents in Minutes (n8n Tutorial)," YouTube, 2024. [Online]. Available: https://www.youtube.com/watch?v=4kXisbPnNsU

[72] "How to Build Smarter RAG Database Agents (n8n)," YouTube, 2024. [Online]. Available: https://www.youtube.com/watch?v=l3d7Zw2bbCw
