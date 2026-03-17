# 🐱 Cat Feeder Backend

Local-first REST API backend for the smart cat feeder mechatronics project. Built with **Node.js + Express + SQLite**, designed to integrate with an ESP32 via a local MQTT broker.

---

## Prerequisites

- Node.js ≥ 18
- `sqlite3` CLI (optional, for schema inspection)

## Quick Start

```bash
# 1. Install dependencies
npm install

# 2. Copy environment config
cp .env.example .env

# 3. Start dev server (DB is initialized automatically on boot)
npm run dev
```

Server starts at **http://localhost:3000**

---

## Available Scripts

| Command | Description |
|---|---|
| `npm run dev` | Start with nodemon (auto-reload) |
| `npm start` | Start in production mode |
| `npm run init-db` | Run schema init in isolation |

---

## API Endpoints

| Method | Path | Status |
|---|---|---|
| GET \| POST | `/api/cats` | 🔲 Stub |
| GET \| PUT \| DELETE | `/api/cats/:id` | 🔲 Stub |
| GET \| POST | `/api/dietary-plans` | 🔲 Stub |
| GET \| PUT \| DELETE | `/api/dietary-plans/:id` | 🔲 Stub |
| GET \| POST | `/api/dietary-plans/:id/schedules` | 🔲 Stub |
| GET \| POST | `/api/sensor-logs` | 🔲 Stub |
| GET \| PUT | `/api/hardware-settings` | 🔲 Stub |
| GET | `/health` | ✅ Live |

---

## Database Schema

```
cats               ←→ dietary_plans ←→ schedules
                             sensor_logs (append-only telemetry)
                             hardware_settings (single-row ESP32 config)
```

Inspect with:
```bash
sqlite3 database.sqlite ".tables"
sqlite3 database.sqlite ".schema"
```

---

## Project Structure

```
src/
├── app.js               ← Express entry point
├── db/
│   ├── database.js      ← Shared DB connection (better-sqlite3)
│   └── init.js          ← Schema creation script
├── routes/              ← Route definitions
└── controllers/         ← Request handlers (stubs)
```
