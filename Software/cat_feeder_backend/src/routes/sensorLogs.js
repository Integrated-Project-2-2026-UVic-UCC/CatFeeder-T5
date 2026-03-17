'use strict';

const express = require('express');
const router = express.Router();
const ctrl = require('../controllers/sensorLogsController');

router.get('/', ctrl.getLogs);
router.post('/', ctrl.createLog);

module.exports = router;
