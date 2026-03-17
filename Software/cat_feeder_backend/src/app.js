'use strict';

require('dotenv').config();
const express = require('express');
const cors = require('cors');

// Initialize DB schema on startup
require('./db/init');

// Route modules
const catsRouter = require('./routes/cats');
const dietaryPlansRouter = require('./routes/dietaryPlans');
const sensorLogsRouter = require('./routes/sensorLogs');
const hardwareSettingsRouter = require('./routes/hardwareSettings');

// MQTT service
const { connectMqtt } = require('./services/mqttService');

const app = express();
const PORT = process.env.PORT || 3000;

// ─── Middleware ────────────────────────────────────────────────────────────────
app.use(cors());
app.use(express.json());

// ─── Health Check ─────────────────────────────────────────────────────────────
app.get('/health', (req, res) => {
    res.json({
        status: 'ok',
        service: 'cat-feeder-backend',
        timestamp: new Date().toISOString(),
    });
});

// ─── API Routes ───────────────────────────────────────────────────────────────
app.use('/api/cats', catsRouter);
app.use('/api/dietary-plans', dietaryPlansRouter);
app.use('/api/sensor-logs', sensorLogsRouter);
app.use('/api/hardware-settings', hardwareSettingsRouter);

// ─── 404 Handler ─────────────────────────────────────────────────────────────
app.use((req, res) => {
    res.status(404).json({ error: 'Route not found' });
});

// ─── Global Error Handler ─────────────────────────────────────────────────────
app.use((err, req, res, next) => { // eslint-disable-line no-unused-vars
    console.error('❌ Unhandled error:', err);
    res.status(500).json({ error: 'Internal server error', detail: err.message });
});

// ─── Start ────────────────────────────────────────────────────────────────────
app.listen(PORT, () => {
    console.log(`🐱 Cat Feeder API listening on http://localhost:${PORT}`);
    console.log(`   Health: http://localhost:${PORT}/health`);
    connectMqtt();
});

module.exports = app;
