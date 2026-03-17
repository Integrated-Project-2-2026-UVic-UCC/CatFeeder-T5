'use strict';

const mqtt = require('mqtt');
const db = require('../db/database');

// ─── Config ───────────────────────────────────────────────────────────────────
const BROKER_URL = process.env.MQTT_BROKER_URL || 'mqtt://localhost:1883';

const TOPIC_TELEMETRY = 'catfeeder/telemetry';
const TOPIC_JAM_ALERT = 'catfeeder/alerts/jam';

// ─── Prepared Statements ──────────────────────────────────────────────────────
const insertTelemetry = db.prepare(`
  INSERT INTO sensor_logs (current_weight_grams, temperature, humidity, motor_status)
  VALUES (@weight, @temperature, @humidity, @motor_status)
`);

// ─── Handlers ─────────────────────────────────────────────────────────────────

/**
 * Handles a message on catfeeder/telemetry.
 * Expected payload: { "weight": 45.2, "temperature": 22.1, "humidity": 58.0 }
 */
function handleTelemetry(payload) {
    let data;
    try {
        data = JSON.parse(payload.toString());
    } catch (err) {
        console.error('❌ [MQTT] Failed to parse telemetry payload:', err.message);
        return;
    }

    const { weight = null, temperature = null, humidity = null } = data;

    try {
        insertTelemetry.run({
            weight,
            temperature,
            humidity,
            motor_status: null,
        });
        console.log(`✅ [MQTT] Telemetry logged — weight: ${weight}g, temp: ${temperature}°C, humidity: ${humidity}%`);
    } catch (err) {
        console.error('❌ [MQTT] DB insert failed for telemetry:', err.message);
    }
}

/**
 * Handles a message on catfeeder/alerts/jam.
 * Expected payload: { "status": "jammed" }
 */
function handleJamAlert(payload) {
    let data;
    try {
        data = JSON.parse(payload.toString());
    } catch (err) {
        console.error('❌ [MQTT] Failed to parse jam-alert payload:', err.message);
        return;
    }

    console.warn(`🚨 [MQTT] Jam alert received — status: "${data.status}"`);

    try {
        insertTelemetry.run({
            weight: null,
            temperature: null,
            humidity: null,
            motor_status: 'error',
        });
        console.log('✅ [MQTT] Jam alert logged to sensor_logs (motor_status = error)');
    } catch (err) {
        console.error('❌ [MQTT] DB insert failed for jam alert:', err.message);
    }
}

// ─── Connect ──────────────────────────────────────────────────────────────────

/**
 * Connects to the MQTT broker and sets up topic subscriptions.
 * Call this once after the HTTP server is listening.
 */
function connectMqtt() {
    const client = mqtt.connect(BROKER_URL, {
        clientId: `cat_feeder_backend_${Date.now()}`,
        clean: true,
        reconnectPeriod: 5000,   // retry every 5 s on disconnect
    });

    client.on('connect', () => {
        console.log(`🟢 [MQTT] Connected to ${BROKER_URL}`);

        client.subscribe(TOPIC_TELEMETRY, { qos: 1 }, (err) => {
            if (err) console.error(`❌ [MQTT] Subscribe failed (${TOPIC_TELEMETRY}):`, err.message);
            else console.log(`📡 [MQTT] Subscribed to ${TOPIC_TELEMETRY}`);
        });

        client.subscribe(TOPIC_JAM_ALERT, { qos: 1 }, (err) => {
            if (err) console.error(`❌ [MQTT] Subscribe failed (${TOPIC_JAM_ALERT}):`, err.message);
            else console.log(`📡 [MQTT] Subscribed to ${TOPIC_JAM_ALERT}`);
        });
    });

    client.on('message', (topic, payload) => {
        if (topic === TOPIC_TELEMETRY) {
            handleTelemetry(payload);
        } else if (topic === TOPIC_JAM_ALERT) {
            handleJamAlert(payload);
        }
    });

    client.on('reconnect', () => {
        console.log('🔄 [MQTT] Reconnecting to broker...');
    });

    client.on('error', (err) => {
        console.error('❌ [MQTT] Connection error:', err.message);
    });

    client.on('close', () => {
        console.warn('🔴 [MQTT] Connection closed.');
    });

    return client;
}

module.exports = { connectMqtt };
