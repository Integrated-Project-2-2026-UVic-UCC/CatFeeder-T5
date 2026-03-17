'use strict';

/**
 * Hardware Settings Controller
 * All handlers return 501 until implemented.
 */

exports.getSettings = (req, res) => {
    res.status(501).json({ status: 'not_implemented', message: 'GET /api/hardware-settings — coming soon' });
};

exports.updateSettings = (req, res) => {
    res.status(501).json({ status: 'not_implemented', message: 'PUT /api/hardware-settings — coming soon' });
};
