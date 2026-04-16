# CatFeeder - Software Components

This directory contains the entire software stack for the smart CatFeeder project, consisting of a production-grade web application, an IoT firmware simulation, and supporting design resources.

## Architecture Overview

The software is divided into the following main functional components:

1.  **Web Application (`webapp/`)**: A React-based interface allowing users to monitor their cat's data, manage feeding schedules, and interact with the physical device.
2.  **Firmware Mock (`firmware/`)**: An Arduino C++ firmware for the ESP32 that completely mocks the physical sensors and actuators (to be replaced with actual physical components).
3.  **Agent Resources (`agent/`)**: Skills and guidelines focusing on strong frontend design aesthetic and prompt specifications.
4.  **Specifications & Design Assets**:
    *   `SRS_CatFeeder_WebApp.docx.md`: The Software Requirements Specification outlining system capabilities.
    *   `diseño_ui.png`: High-fidelity UI design reference.

---

## 1. Web Application (`webapp/`)

A responsive, bespoke frontend application designed with a strong emphasis on intentional aesthetics and smooth performance. It serves as the primary control panel for the project.

### Tech Stack & Libraries
*   **React 19 & Vite**: The core foundation, providing blazing-fast initialization and HMR.
*   **React Router v7**: For robust client-side routing across sections (*Dashboard, Cats, Schedules, History, Device, Settings, Chat*).
*   **Supabase Client**: Provides BaaS capabilities for PostgreSQL database interactions, real-time subscriptions, and authentication (`@supabase/supabase-js`).
*   **Zustand**: A small, fast, and scalable bearbones state-management solution for handling authentication sessions and global UI state.
*   **Recharts**: Used for rendering visual data analytics, such as the history of cat feeding patterns and weight variations.
*   **Vanilla CSS**: Custom styling following strict frontend design mandates. Prevents over-reliance on utility frameworks, achieving a distinct and premium interface.

### Application Workflow
1.  **Auth & Session**: Handled globally in `App.jsx` using `useAuthStore` (Zustand). Only authenticated users are routed past the `<PrivateRoute>` wrapper into the layout.
2.  **Database Integration**: Pages execute queries using the Supabase client to fetch table information (`cats`, `schedules`, `commands`, `devices`, `feed_events`).
3.  **Hardware Control**: To dispense food or tare the scale, the webapp pushes a new row into the `commands` table. The ESP32 device actively listens to this table.
4.  **Real-time Monitoring**: The dashboard visualizes telemetry (temperature, humidity, scale weight) inserted into Supabase by the physical device.

### Running Locally
You will need Node.js installed. In your terminal:
```bash
cd webapp
npm install 
npm run dev
```
*(Ensure `.env` contains your `VITE_SUPABASE_URL` and `VITE_SUPABASE_ANON_KEY` variables to successfully interact with the database).*

---

## 2. Firmware Simulation (`firmware/`)

Located at `firmware/CatFeeder_Mock/CatFeeder_Mock.ino`, this Arduino C++ instance ensures full end-to-end functionality of the platform without needing the final hardware assembly.

### Core Features
*   **Target Board**: Designed for ESP32 (WROOM-32).
*   **Direct Cloud Integration**: Connects straight to the Supabase REST API via `HTTPClient` and `WiFiClientSecure` using an API JWT authorization header.
*   **Bi-directional Communication**:
    *   **Polling (GET)**: Fetches `device_config` (every 60s) and checks the `commands` table for pending actions (every 5s).
    *   **Pushing (POST/PATCH)**: Sends `realtime_weight` during active operations (`500ms`) and updates device `heartbeat` statuses.
*   **Virtual Sensors & Actuators**: Calculates pseudo-random Gaussian noise behavior across variables:
    *   **Scale**: Mimics the HX711 Load Cell behaviors.
    *   **Environment**: Simulates a DHT22 module to relay Temperature & Humidity.
    *   **Dispenser Motor**: Simulates DRV8825 logic with timeout safety guards, "dispensing" at a fake `8.0g/s` feed rate.

### Usage Setup
1.  Open `CatFeeder_Mock.ino` with the Arduino IDE. 
2.  Install the **ArduinoJson** (`v7.x`) library from the Library Manager.
3.  Update the network credentials (`WIFI_SSID`, `WIFI_PASS`) and endpoint configs (`SUPABASE_URL`, `SUPABASE_KEY`, `DEVICE_ID`).
4.  Upload to your ESP32 board and open the Serial Monitor at `115200` baud.

---

## 3. Architectural Note

This software adopts an asynchronous, database-as-the-broker architecture. The web application and the ESP32 never communicate with each other directly; they only write to or listen to the Supabase database. This guarantees a decoupled, resilient system. Row Level Security (RLS) rules in Supabase must be set up properly to make sure IoT requests only interact with permitted device rows.
