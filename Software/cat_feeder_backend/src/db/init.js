const db = require('./database');

const initializeDatabase = () => {
    console.log('📦 Initializing database schema...');

    // -----------------------------------------------------------------------
    // Table: cats
    // -----------------------------------------------------------------------
    db.exec(`
    CREATE TABLE IF NOT EXISTS cats (
      id          INTEGER PRIMARY KEY AUTOINCREMENT,
      name        TEXT    NOT NULL,
      age         REAL,
      weight      REAL,
      breed       TEXT,
      rfid_tag_id TEXT    UNIQUE,
      created_at  TEXT    DEFAULT (datetime('now'))
    );
  `);

    // -----------------------------------------------------------------------
    // Table: dietary_plans
    // One active plan per cat; parent of schedules (ON DELETE CASCADE)
    // -----------------------------------------------------------------------
    db.exec(`
    CREATE TABLE IF NOT EXISTS dietary_plans (
      id                  INTEGER PRIMARY KEY AUTOINCREMENT,
      cat_id              INTEGER NOT NULL REFERENCES cats(id) ON DELETE CASCADE,
      daily_target_grams  REAL    NOT NULL,
      active_status       INTEGER NOT NULL DEFAULT 1,
      notes               TEXT,
      created_at          TEXT    DEFAULT (datetime('now'))
    );
  `);

    // -----------------------------------------------------------------------
    // Table: schedules
    // Up to 10 configurable time slots per dietary plan (min 6 required)
    // -----------------------------------------------------------------------
    db.exec(`
    CREATE TABLE IF NOT EXISTS schedules (
      id              INTEGER PRIMARY KEY AUTOINCREMENT,
      dietary_plan_id INTEGER NOT NULL REFERENCES dietary_plans(id) ON DELETE CASCADE,
      slot_number     INTEGER NOT NULL CHECK (slot_number BETWEEN 1 AND 10),
      feed_time       TEXT    NOT NULL,
      portion_grams   REAL    NOT NULL,
      enabled         INTEGER NOT NULL DEFAULT 1,
      UNIQUE (dietary_plan_id, slot_number)
    );
  `);

    // -----------------------------------------------------------------------
    // Table: sensor_logs
    // Append-only time-series telemetry from the ESP32
    // -----------------------------------------------------------------------
    db.exec(`
    CREATE TABLE IF NOT EXISTS sensor_logs (
      id                    INTEGER PRIMARY KEY AUTOINCREMENT,
      timestamp             TEXT    DEFAULT (datetime('now')),
      current_weight_grams  REAL,
      temperature           REAL,
      humidity              REAL,
      motor_status          TEXT    CHECK (motor_status IN ('idle', 'running', 'error'))
    );
  `);

    // Index for fast time-range queries on growing telemetry log
    db.exec(`
    CREATE INDEX IF NOT EXISTS idx_sensor_logs_timestamp
    ON sensor_logs (timestamp);
  `);

    // -----------------------------------------------------------------------
    // Table: hardware_settings
    // Single-row config; ESP32 always reads/writes row id = 1
    // -----------------------------------------------------------------------
    db.exec(`
    CREATE TABLE IF NOT EXISTS hardware_settings (
      id                  INTEGER PRIMARY KEY AUTOINCREMENT,
      calibration_factor  REAL    NOT NULL DEFAULT 1.0,
      motor_speed         INTEGER NOT NULL DEFAULT 100,
      motor_steps         INTEGER NOT NULL DEFAULT 200,
      updated_at          TEXT    DEFAULT (datetime('now'))
    );
  `);

    // Seed the single default row (INSERT OR IGNORE is idempotent)
    db.exec(`INSERT OR IGNORE INTO hardware_settings (id) VALUES (1);`);

    console.log('✅ Database schema initialized successfully.');
    console.log('   Tables: cats, dietary_plans, schedules, sensor_logs, hardware_settings');
};

initializeDatabase();
