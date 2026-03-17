'use strict';

const express = require('express');
const router = express.Router();
const ctrl = require('../controllers/dietaryPlansController');

router.get('/', ctrl.getAllPlans);
router.get('/:id', ctrl.getPlanById);
router.post('/', ctrl.createPlan);
router.put('/:id', ctrl.updatePlan);
router.delete('/:id', ctrl.deletePlan);

// Nested schedule slots routes
router.get('/:id/schedules', ctrl.getSchedulesForPlan);
router.post('/:id/schedules', ctrl.addScheduleSlot);

module.exports = router;
