'use strict';

/**
 * Sensor Logs Controller
 * All handlers return 501 until implemented.
 */

exports.getLogs = (req, res) => {
    res.status(501).json({ status: 'not_implemented', message: 'GET /api/sensor-logs — coming soon' });
};

exports.createLog = (req, res) => {
    res.status(501).json({ status: 'not_implemented', message: 'POST /api/sensor-logs — coming soon' });
};
