'use strict';

/**
 * Dietary Plans Controller
 * All handlers return 501 until implemented.
 */

exports.getAllPlans = (req, res) => {
    res.status(501).json({ status: 'not_implemented', message: 'GET /api/dietary-plans — coming soon' });
};

exports.getPlanById = (req, res) => {
    res.status(501).json({ status: 'not_implemented', message: `GET /api/dietary-plans/${req.params.id} — coming soon` });
};

exports.createPlan = (req, res) => {
    res.status(501).json({ status: 'not_implemented', message: 'POST /api/dietary-plans — coming soon' });
};

exports.updatePlan = (req, res) => {
    res.status(501).json({ status: 'not_implemented', message: `PUT /api/dietary-plans/${req.params.id} — coming soon` });
};

exports.deletePlan = (req, res) => {
    res.status(501).json({ status: 'not_implemented', message: `DELETE /api/dietary-plans/${req.params.id} — coming soon` });
};

// --- Nested: Schedules ---

exports.getSchedulesForPlan = (req, res) => {
    res.status(501).json({ status: 'not_implemented', message: `GET /api/dietary-plans/${req.params.id}/schedules — coming soon` });
};

exports.addScheduleSlot = (req, res) => {
    res.status(501).json({ status: 'not_implemented', message: `POST /api/dietary-plans/${req.params.id}/schedules — coming soon` });
};
