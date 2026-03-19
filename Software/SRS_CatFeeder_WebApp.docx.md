**SOFTWARE REQUIREMENTS SPECIFICATION**

**Cat Feeder Web Application**

ESP32 Communication & Autonomous Control Interface

| Integrated Project II  |  GR15 \[GEMEC-09UV\] Universitat de Vic – Universitat Central de Catalunya Academic Year 2025 – 2026 |
| :---: |

| Document Title | Software Requirements Specification – Cat Feeder Web App |
| :---- | :---- |
| Version | 1.0 |
| Date | March 2026 |
| Status | Draft – For Review |
| Project Manager | Biel Tomas Rifa |
| Electronics Lead | Georgina Garcia Vilaseca |
| Software Lead | Ferran Vila Ordeig |
| Mechanical Lead | Pol Roch Jose |

# **1\. Introduction**

## **1.1 Purpose**

This Software Requirements Specification (SRS) defines the functional and non-functional requirements for the Cat Feeder Web Application (CFWA). The application provides a user-friendly web interface to remotely configure, monitor, and control an autonomous cat feeder based on an ESP32 microcontroller, storing all operational data on a Supabase backend.

This document is intended for the project team at Universitat de Vic – UCC and serves as the primary reference for design, development, and validation activities.

## **1.2 Scope**

The Cat Feeder Web Application shall:

* Allow registered users to manage one or more cats and their individual feeding profiles.

* Schedule and trigger autonomous food dispensing via the ESP32 device.

* Monitor real-time dispensed weight using the load cell (HX711) data reported by the ESP32.

* Store all configuration, events, and historical data in Supabase (PostgreSQL).

* Use Supabase Auth for secure user authentication and authorization.

* Communicate with the ESP32 using a well-defined, lightweight messaging protocol (MQTT or REST over Wi-Fi).

## **1.3 Definitions, Acronyms, and Abbreviations**

| Term / Acronym | Definition |
| ----- | ----- |
| SRS | Software Requirements Specification |
| CFWA | Cat Feeder Web Application |
| ESP32 | Espressif Systems microcontroller used as the embedded control unit |
| HX711 | 24-bit ADC module for reading the load cell weight sensor |
| MQTT | Message Queuing Telemetry Transport — lightweight IoT publish/subscribe protocol |
| REST | Representational State Transfer — HTTP-based API architecture |
| Supabase | Open-source Firebase alternative providing PostgreSQL DB, Auth, and Realtime APIs |
| PWA | Progressive Web App — web application installable on mobile/desktop devices |
| RLS | Row Level Security — Supabase/PostgreSQL policy-based data access control |
| Feed Event | A single dispensing cycle: scheduled or manual, with target and actual weight |
| Portion | Target weight (grams) of food to be dispensed in one feed event |

## **1.4 References**

* README.md – Automatic Cat Feeder, Integrated Project II, GR15 \[GEMEC-09UV\]

* README\_Electronics.md – Electronics Subsystem Documentation

* ESP32 Technical Reference Manual – Espressif Systems

* HX711 Datasheet – Avia Semiconductor

* Supabase Documentation – https://supabase.com/docs

* MQTT Protocol Specification v5.0 – OASIS Standard

* ANFAAC – Sectoral Data on Pet Ownership

## **1.5 Overview**

This document is organized as follows: Section 2 provides an overall description of the system. Section 3 details specific functional requirements. Section 4 covers non-functional requirements. Section 5 describes the data model. Section 6 specifies the ESP32 communication protocol. Section 7 defines the user interface requirements. Section 8 covers security requirements, and Section 9 presents constraints and future scalability.

# **2\. Overall Description**

## **2.1 Product Perspective**

The CFWA is the remote control and monitoring front-end for the autonomous cat feeder hardware system. The hardware (ESP32-based) operates independently but accepts commands and reports status to the web application. The system forms a client-firmware architecture:

* The web application (client) runs in the user's browser and optionally as a PWA on mobile.

* Supabase provides the backend infrastructure: database, authentication, realtime subscriptions, and edge functions.

* The ESP32 device connects to the same Supabase project via REST or MQTT to receive scheduling configuration and to push weight/event data.

## **2.2 Product Functions – High Level**

| \# | Feature Area | Summary |
| ----- | ----- | ----- |
| F-01 | User Authentication | Register, login, logout, password reset via Supabase Auth |
| F-02 | Cat Profile Management | Create, edit, delete cats with photo, name, weight, age, breed, diet notes |
| F-03 | Feeding Schedule | Define recurring or one-time schedules per cat with portion size |
| F-04 | Manual Dispensing | Trigger an immediate dispensing command to the ESP32 |
| F-05 | Real-time Weight Monitor | Live display of dispensed food weight via HX711/ESP32 stream |
| F-06 | Feed History & Analytics | Log of all feed events with charts and statistics |
| F-07 | Device Management | Register and configure the ESP32 device, status monitoring |
| F-08 | Alerts & Notifications | Low food level, motor fault, missed feed, offline device alerts |
| F-09 | Multi-cat Support | Assign schedules and portions individually per cat |
| F-10 | Settings & Calibration | Tare the scale, calibrate weight unit, adjust motor run time |

## **2.3 User Classes and Characteristics**

### **2.3.1 Owner (Primary User)**

The primary user is a pet owner with no required technical background. They interact daily with the feeding schedules, monitor cat health trends, and receive notifications. The interface must be intuitive and mobile-friendly.

### **2.3.2 Administrator**

The administrator (typically the project team during deployment) can access device configuration, firmware update triggers, and system-level settings. Admin functions are hidden from standard users and protected by role-based access control enforced at the Supabase RLS level.

### **2.3.3 ESP32 Device (System Actor)**

The embedded device acts as an authenticated service actor. It authenticates using a Supabase service key or a dedicated device JWT and publishes weight/event data while subscribing to schedule updates.

## **2.4 Operating Environment**

* Web application: Modern browsers (Chrome 100+, Firefox 100+, Safari 15+, Edge 100+)

* Mobile: PWA installable on iOS 15+ and Android 10+

* Backend: Supabase cloud (PostgreSQL 15, GoTrue Auth, Realtime engine, Storage)

* ESP32: Wi-Fi 802.11 b/g/n, Arduino framework or ESP-IDF, MQTT / HTTP client

* Network: Both CFWA and ESP32 require internet/LAN connectivity

## **2.5 Assumptions and Dependencies**

* The ESP32 is always on the same network or internet-connected to reach Supabase.

* Supabase free/pro tier provides sufficient throughput for the expected event rate (\<100 events/day).

* The load cell and HX711 are correctly calibrated before first use via the web settings panel.

* A stable Wi-Fi network is available at the feeder installation location.

* Users have a valid email address for Supabase Auth registration.

# **3\. Specific Functional Requirements**

## **3.1 Authentication & Authorization (F-01)**

### **3.1.1 User Registration**

* REQ-01-01: The system shall allow a new user to register using an email address and password via Supabase Auth.

* REQ-01-02: The system shall send a confirmation email and activate the account only after email verification.

* REQ-01-03: Passwords shall be at least 8 characters, containing at least one uppercase letter and one number.

### **3.1.2 Login & Session Management**

* REQ-01-04: The system shall allow registered users to log in with their email and password.

* REQ-01-05: The system shall support session persistence using Supabase JWT tokens stored securely (httpOnly cookies or localStorage with PKCE flow).

* REQ-01-06: Sessions shall expire after 7 days of inactivity, requiring re-authentication.

* REQ-01-07: The system shall provide a password reset flow via email.

### **3.1.3 Authorization**

* REQ-01-08: All database tables shall have Supabase Row Level Security (RLS) policies enforcing that each user can only read and write their own data.

* REQ-01-09: Admin-only routes and functions shall be protected by a role claim in the user JWT.

## **3.2 Cat Profile Management (F-02)**

### **3.2.1 Create & Edit Cat Profile**

* REQ-02-01: The system shall allow a user to create one or more cat profiles, each with the following fields: name (required), breed (optional), date of birth (optional), weight in kg (optional), dietary notes (optional), photo (optional, stored in Supabase Storage).

* REQ-02-02: The system shall allow the user to edit all profile fields at any time.

* REQ-02-03: The system shall allow the user to archive or delete a cat profile. Deleting a profile shall prompt a confirmation dialog.

* REQ-02-04: The system shall display all cats in a visual card-based dashboard overview.

### **3.2.2 Cat Identification**

* REQ-02-05: Each cat profile shall be assigned a unique UUID generated by Supabase upon creation.

* REQ-02-06: The system shall support identifying the feeding cat through a manual selection in the app (multi-cat households: user selects which cat is eating before a manual feed event).

## **3.3 Feeding Schedule Management (F-03)**

### **3.3.1 Creating Schedules**

* REQ-03-01: The system shall allow the user to create feeding schedules assigned to a specific cat.

* REQ-03-02: Each schedule entry shall define: target cat, scheduled time (HH:MM), days of the week (recurring) or a specific date (one-time), and target portion size in grams.

* REQ-03-03: The system shall allow a minimum portion of 5 g and a maximum of 500 g per schedule entry.

* REQ-03-04: The system shall allow multiple schedule entries per cat per day.

### **3.3.2 Activating & Deactivating Schedules**

* REQ-03-05: Each schedule entry shall have an enabled/disabled toggle that can be changed in real time without deleting the entry.

* REQ-03-06: Disabling a schedule shall immediately be reflected in the ESP32 configuration (next sync cycle, max 60 seconds delay).

### **3.3.3 Schedule Synchronization to ESP32**

* REQ-03-07: When a schedule is created, modified, or deleted, the system shall update the device\_config table in Supabase, which the ESP32 polls/subscribes to.

* REQ-03-08: The ESP32 shall be able to operate autonomously using its last known schedule in the event of network disconnection.

## **3.4 Manual Dispensing (F-04)**

* REQ-04-01: The system shall provide a 'Feed Now' button on the dashboard and on each cat's detail page.

* REQ-04-02: Pressing 'Feed Now' shall open a confirmation dialog allowing the user to select the target cat and the portion size before confirming.

* REQ-04-03: Upon confirmation, the system shall insert a pending command record in the Supabase commands table with status 'pending'.

* REQ-04-04: The ESP32 shall detect the pending command, execute the dispensing cycle, and update the record status to 'completed' or 'error'.

* REQ-04-05: The UI shall reflect the command status in real time using Supabase Realtime subscriptions.

* REQ-04-06: A manual feed command shall be rejected if a scheduled feed cycle is already in progress (debounce protection).

## **3.5 Real-time Weight Monitoring (F-05)**

* REQ-05-01: During an active dispensing cycle, the application shall display the current dispensed weight in grams in real time, updated every 500 ms.

* REQ-05-02: The weight display shall show a progress bar indicating dispensed weight vs. target portion.

* REQ-05-03: The ESP32 shall publish weight readings to the Supabase realtime\_weight table or via MQTT, which the web app subscribes to.

* REQ-05-04: The system shall display 'Motor running', 'Target reached', 'Idle', or 'Fault' as device state indicators.

* REQ-05-05: A manual tare (zero) command shall be available from the settings screen to send a tare instruction to the ESP32.

## **3.6 Feed History & Analytics (F-06)**

* REQ-06-01: Every completed or failed dispensing cycle shall be stored as a feed event record in Supabase with: timestamp, cat ID, target portion, actual dispensed weight, trigger type (scheduled/manual), and status.

* REQ-06-02: The system shall display a paginated feed history list, filterable by cat and date range.

* REQ-06-03: The system shall display the following analytics charts: daily food intake per cat (last 7/30 days), weekly average portion accuracy (target vs. actual), number of feeds per day/week.

* REQ-06-04: The system shall allow the user to export feed history as a CSV file.

## **3.7 Device Management (F-07)**

* REQ-07-01: The system shall allow the user to register their ESP32 device using a unique device ID (MAC address or provisioned UUID).

* REQ-07-02: The system shall display the device's last-seen timestamp, connection status (online/offline), and firmware version.

* REQ-07-03: The system shall allow the user to rename the device.

* REQ-07-04: The system shall allow the user to perform a remote restart command (soft reset) on the ESP32.

* REQ-07-05: The system shall display motor health status (normal, blocked, overtemperature) as reported by the ESP32.

## **3.8 Alerts & Notifications (F-08)**

* REQ-08-01: The system shall send an in-app notification and optionally an email when a scheduled feed event is missed (ESP32 did not confirm execution within 5 minutes of scheduled time).

* REQ-08-02: The system shall alert the user when the food hopper is estimated to be low (based on cumulative dispensed weight since last refill).

* REQ-08-03: The system shall alert the user when the ESP32 device goes offline for more than 10 minutes.

* REQ-08-04: The system shall alert the user when a motor fault is reported by the ESP32.

* REQ-08-05: Notification preferences (in-app, email) shall be configurable per user in the settings panel.

## **3.9 Multi-cat Support (F-09)**

* REQ-09-01: The system shall support management of multiple cats per user account with independent schedules and portion sizes.

* REQ-09-02: The dashboard shall display a summary card per cat showing next scheduled feed, last feed timestamp, and last dispensed weight.

* REQ-09-03: Feed history and analytics shall be viewable both globally (all cats) and filtered per individual cat.

## **3.10 Settings & Calibration (F-10)**

* REQ-10-01: The system shall allow the user to set the unit weight calibration factor for the HX711 load cell by entering a known reference weight.

* REQ-10-02: The system shall allow the user to trigger a tare (zero reset) of the scale remotely.

* REQ-10-03: The system shall allow the user to configure the motor run-time safety limit (maximum seconds per dispensing cycle) to prevent mechanical overrun.

* REQ-10-04: The system shall allow the user to configure the hopper capacity (grams) for low-food estimation.

* REQ-10-05: All calibration and settings changes shall be pushed to the ESP32 via the Supabase device\_config record.

# **4\. Non-Functional Requirements**

## **4.1 Performance**

* REQ-NF-01: The web application shall load the main dashboard within 3 seconds on a standard 10 Mbps connection.

* REQ-NF-02: Real-time weight updates shall have an end-to-end latency of no more than 1 second from the ESP32 measurement to the UI update.

* REQ-NF-03: Manual feed commands shall be acknowledged by the ESP32 within 5 seconds under normal network conditions.

* REQ-NF-04: The database shall support at least 100 concurrent users without degradation, leveraging Supabase connection pooling (PgBouncer).

## **4.2 Usability**

* REQ-NF-05: The user interface shall be fully responsive, providing an equivalent experience on desktop (1280px+) and mobile (360px+) screen widths.

* REQ-NF-06: All primary user workflows (schedule creation, manual feed, view history) shall be completable in no more than 3 navigation steps from the dashboard.

* REQ-NF-07: The application shall comply with WCAG 2.1 Level AA accessibility guidelines.

* REQ-NF-08: All error messages shall be written in plain language and suggest corrective actions.

## **4.3 Reliability & Availability**

* REQ-NF-09: The web application shall target 99.5% uptime, relying on Supabase's hosted infrastructure SLA.

* REQ-NF-10: The ESP32 shall continue executing locally stored schedules even if the internet connection is lost, ensuring feeding continuity.

* REQ-NF-11: All feed events shall be stored with at least the actual dispensed weight, even if the target configuration is lost.

## **4.4 Security**

* REQ-NF-12: All communication between the browser, Supabase, and ESP32 shall use TLS 1.2 or higher (HTTPS/MQTTS).

* REQ-NF-13: User passwords shall be hashed using bcrypt via Supabase Auth (GoTrue); plaintext passwords shall never be stored.

* REQ-NF-14: API keys and secrets shall never be exposed in client-side code. ESP32 shall use a restricted Supabase anon key with RLS enforcing device-scope access.

* REQ-NF-15: The application shall implement CSRF protection and Content Security Policy (CSP) headers.

## **4.5 Maintainability & Scalability**

* REQ-NF-16: The front-end shall be developed using a component-based framework (React or Vue) with modular architecture.

* REQ-NF-17: The codebase shall include unit tests for all business-logic functions (coverage \>= 70%).

* REQ-NF-18: The data model shall be designed to support addition of new sensor types (temperature, water level) without breaking existing functionality.

* REQ-NF-19: The communication protocol between CFWA and ESP32 shall be versioned to allow firmware upgrades without breaking the app.

# **5\. Data Model (Supabase / PostgreSQL)**

## **5.1 Entity Overview**

| Table | Description |
| ----- | ----- |
| users | Managed by Supabase Auth (auth.users). Extended with public.profiles. |
| profiles | User display name, preferences, notification settings. |
| cats | Cat profiles (name, breed, dob, weight, photo\_url, owner\_id). |
| devices | ESP32 device records (device\_id, name, last\_seen, firmware\_ver, status). |
| device\_config | Live configuration pushed to ESP32 (schedules JSON, motor limits, calibration). |
| schedules | Recurring/one-time feeding schedule entries per cat. |
| commands | Pending/completed/error manual commands sent to ESP32. |
| feed\_events | Full log of every dispensing cycle (timestamp, cat\_id, target\_g, actual\_g, trigger, status). |
| realtime\_weight | Ephemeral table for live weight streaming from ESP32 during active cycle. |
| notifications | In-app notification records (type, message, read status, created\_at). |

## **5.2 Key Table Schemas**

### **5.2.1 cats**

| Column | Type | Description |
| ----- | ----- | ----- |
| id | uuid PK | Auto-generated UUID |
| owner\_id | uuid FK | References auth.users(id), RLS enforced |
| name | text NOT NULL | Cat display name |
| breed | text | Optional breed information |
| date\_of\_birth | date | Optional birth date |
| weight\_kg | numeric(4,2) | Optional current weight in kilograms |
| diet\_notes | text | Veterinary or dietary observations |
| photo\_url | text | Supabase Storage URL of cat photo |
| archived | boolean | Soft-delete flag, default false |
| created\_at | timestamptz | Record creation timestamp |

### **5.2.2 schedules**

| Column | Type | Description |
| ----- | ----- | ----- |
| id | uuid PK | Auto-generated UUID |
| cat\_id | uuid FK | References cats(id) |
| device\_id | uuid FK | References devices(id) |
| time\_of\_day | time NOT NULL | Scheduled feed time (HH:MM:SS) |
| days\_of\_week | int\[\]  | Array of day numbers 0=Sun to 6=Sat (null \= one-time) |
| specific\_date | date | Date for one-time schedules |
| portion\_grams | numeric(6,1) | Target food weight in grams (5–500) |
| enabled | boolean | Active toggle, default true |
| created\_at | timestamptz | Record creation timestamp |

### **5.2.3 feed\_events**

| Column | Type | Description |
| ----- | ----- | ----- |
| id | uuid PK | Auto-generated UUID |
| cat\_id | uuid FK | References cats(id) |
| device\_id | uuid FK | References devices(id) |
| schedule\_id | uuid FK | Null for manual feeds |
| trigger\_type | text | 'scheduled' | 'manual' |
| target\_grams | numeric(6,1) | Configured portion target |
| actual\_grams | numeric(6,1) | Weight actually dispensed (from HX711) |
| status | text | 'completed' | 'partial' | 'error' | 'skipped' |
| error\_code | text | Optional error descriptor from ESP32 |
| started\_at | timestamptz | Cycle start timestamp |
| ended\_at | timestamptz | Cycle end timestamp |

# **6\. ESP32 Communication Protocol**

## **6.1 Overview**

The web application communicates with the ESP32 exclusively through the Supabase backend. Direct browser-to-device communication is not used, ensuring security and NAT traversal compatibility. Two integration patterns are supported:

* Pattern A (Recommended): ESP32 uses the Supabase REST API and Realtime WebSocket client to read device\_config and write feed\_events and realtime\_weight records.

* Pattern B (Alternative): An MQTT broker (e.g., HiveMQ Cloud or Mosquitto) bridges Supabase Realtime events to MQTT topics; the ESP32 subscribes to its topic.

## **6.2 Command Flow – Manual Dispensing**

| Step | Actor | Action |
| ----- | ----- | ----- |
| 1 | Web App | User presses 'Feed Now', selects cat and portion |
| 2 | Web App | Inserts row in commands table: {device\_id, cat\_id, portion\_grams, status: 'pending'} |
| 3 | Supabase Realtime | Broadcasts INSERT event on commands channel to subscribers |
| 4 | ESP32 | Receives event, validates fields, begins motor dispensing cycle |
| 5 | ESP32 | Publishes weight readings every 500 ms to realtime\_weight table |
| 6 | Web App | Subscribes to realtime\_weight for this device\_id, updates live display |
| 7 | ESP32 | On cycle end: inserts feed\_events row, updates command status to 'completed'/'error' |
| 8 | Web App | Receives command status update, shows success/error feedback to user |

## **6.3 Device Configuration Sync**

* The ESP32 polls the device\_config table every 60 seconds and on startup.

* The device\_config record contains a JSON payload with all active schedules, motor limits, calibration factor, and tare state.

* When the web app changes any configurable parameter, it updates device\_config; the ESP32 applies the new configuration on its next poll cycle.

* A config\_version integer field increments on each update; the ESP32 skips processing if the received version matches its local version.

## **6.4 Device Status Reporting**

* The ESP32 updates its devices row (last\_seen, status, firmware\_version) every 30 seconds.

* Possible status values: 'idle', 'dispensing', 'fault\_motor', 'fault\_sensor', 'offline' (set by a Supabase scheduled function if last\_seen is \> 10 minutes ago).

## **6.5 Authentication of the ESP32**

* The ESP32 uses the Supabase anon public key for read access to device\_config.

* Write operations (feed\_events, realtime\_weight, devices) are protected by RLS policies that restrict writes to rows where device\_id matches the device's registered UUID.

* The device UUID is provisioned once during setup via the web application's Device Registration flow and stored in the ESP32 NVS (Non-Volatile Storage).

# **7\. User Interface Requirements**

## **7.1 Application Layout**

* The application shall use a persistent navigation sidebar (desktop) or bottom navigation bar (mobile) with the following top-level sections: Dashboard, My Cats, Schedules, History, Device, Settings.

* The header shall display the application logo, current user email, and a notifications bell icon with an unread count badge.

## **7.2 Dashboard**

* The dashboard shall display one summary card per registered cat showing: cat photo and name, next scheduled feed (time and portion), last feed event (time and actual grams), current device status indicator.

* A global 'Feed Now' floating action button shall be present on the dashboard.

* An active dispensing banner shall appear at the top of the dashboard during any ongoing dispensing cycle, showing live weight progress.

## **7.3 Cat Profile Pages**

* Each cat shall have a detail page displaying all profile fields and a tabbed view for: upcoming schedules, feed history, and statistics charts.

* An edit button shall open an in-page form (no separate page navigation) for modifying profile fields.

## **7.4 Schedule Management Page**

* Schedules shall be displayed in a visual weekly calendar view and a list view (user can toggle).

* A 'New Schedule' button shall open a modal form with all required fields and client-side validation.

* Each schedule row shall have an enable/disable toggle and an edit/delete action menu.

## **7.5 Device Status Page**

* The device page shall display a status card with connection state (color-coded: green \= online, amber \= degraded, red \= offline), last heartbeat timestamp, firmware version, and device name.

* A 'Send Tare' and a 'Restart Device' button shall be visible with confirmation dialogs.

* A raw sensor readings panel shall show the last reported weight from the HX711 in real time.

## **7.6 Visual Design Guidelines**

* Primary color: to be defined by the project team (recommended: a calm blue or green consistent with pet-care branding).

* Typography: Sans-serif font (Inter or similar), minimum 14 px body text.

* All interactive elements shall have a minimum touch target of 44x44 px (WCAG 2.5.5).

* Loading states shall be indicated with skeleton screens, not spinner overlays, to improve perceived performance.

# **8\. Security Requirements**

| ID | Requirement | Rationale |
| ----- | ----- | ----- |
| SEC-01 | Supabase RLS on all tables | Prevent cross-user data leakage at DB level |
| SEC-02 | TLS 1.2+ on all connections | Protect credentials and sensor data in transit |
| SEC-03 | No secrets in client-side code | Anon key is public; service key must never reach the browser |
| SEC-04 | PKCE OAuth flow for auth | Prevent authorization code interception attacks |
| SEC-05 | CSP headers | Mitigate XSS risk from third-party scripts |
| SEC-06 | Rate limiting on commands table | Prevent command flooding / denial of service |
| SEC-07 | Device UUID validation via RLS | ESP32 can only write to its own device\_id rows |
| SEC-08 | Email verification at registration | Prevent fake accounts and unauthorized access |

# **9\. Constraints and Future Scalability**

## **9.1 Current Constraints**

* The system is designed for single-device deployments (one ESP32 per user account). Multi-device per household support is architecturally possible but not in scope for this release.

* Weight measurement accuracy is limited by the HX711 noise floor and load cell quality; the application assumes pre-calibration.

* Scheduled feeding times are based on the ESP32's RTC; clock drift may accumulate if NTP sync is unavailable.

* The mobile app is a PWA; native push notifications require browser notification permissions and are not supported on all iOS versions.

## **9.2 Future Scalability Roadmap**

| Feature | Description |
| ----- | ----- |
| Camera Integration | Add an IP camera feed to visually confirm which cat is eating |
| RFID Cat Identification | Use an RFID reader on the feeder to automatically identify the feeding cat |
| Water Level Monitoring | Add a water sensor and display hydration status in the app |
| Veterinary Export | Generate PDF health reports with food intake trends for vet visits |
| Multi-device Support | Support multiple feeders per household on one account |
| OTA Firmware Updates | Trigger over-the-air ESP32 firmware updates from the web app |
| Voice Assistant Integration | Alexa / Google Home actions to trigger manual feeds by voice |
| AI Portion Recommendation | Suggest optimal portions based on cat weight trends and breed data |

# **10\. Validation & Acceptance Criteria**

Each functional requirement shall be validated according to the criteria defined in this section. Acceptance testing shall be performed jointly by the Software Lead and Project Manager.

| Req. ID | Test Description | Pass Criterion | Method |
| ----- | ----- | ----- | ----- |
| REQ-01-01 | Register new user | Account created, confirmation email received | Manual \+ Auto |
| REQ-03-01 | Create feeding schedule | Schedule persists in DB and appears in device\_config | Manual \+ DB query |
| REQ-04-03 | Manual feed command | Command row inserted with status 'pending' | Automated unit test |
| REQ-04-04 | ESP32 executes command | Command status updates to 'completed' within 10s | Integration test |
| REQ-05-01 | Live weight display | Weight updates visible in UI within 1s of ESP32 publish | Manual observation |
| REQ-06-01 | Feed event logged | feed\_events record created with actual\_grams populated | DB query |
| REQ-08-03 | Device offline alert | Notification appears after 10 min device silence | Simulation test |
| REQ-NF-02 | Realtime latency | Weight update latency \< 1 second measured end-to-end | Timing test |
| SEC-01 | RLS cross-user isolation | User A cannot read User B data via API | Automated security test |

# **Appendix A – Technology Stack Summary**

| Layer | Technology | Notes |
| ----- | ----- | ----- |
| Frontend Framework | React (Vite) or Vue 3 | Component-based, TypeScript recommended |
| UI Component Library | Tailwind CSS \+ shadcn/ui or similar | Responsive, accessible components |
| State Management | Zustand or Pinia | Lightweight, Supabase Realtime integration |
| Charting | Recharts or Chart.js | Feed analytics visualizations |
| Backend / DB | Supabase (PostgreSQL 15\) | Auth, Storage, Realtime, Edge Functions |
| Authentication | Supabase Auth (GoTrue) | Email/password \+ optional OAuth |
| File Storage | Supabase Storage | Cat profile photos |
| Real-time | Supabase Realtime (WebSocket) | Weight streaming, command status |
| Embedded Client | ESP32 (Arduino / ESP-IDF) | HX711 \+ motor driver \+ Wi-Fi |
| ESP32 HTTP Client | ESP32 HTTPClient / ArduinoHttpClient | Supabase REST API calls |
| Deployment | Vercel / Netlify (frontend) | CI/CD from GitHub repository |

**Document end – Software Requirements Specification v1.0**