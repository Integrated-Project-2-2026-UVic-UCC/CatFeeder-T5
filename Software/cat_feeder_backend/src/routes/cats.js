'use strict';

const express = require('express');
const router = express.Router();
const ctrl = require('../controllers/catsController');

router.get('/', ctrl.getAllCats);
router.get('/:id', ctrl.getCatById);
router.post('/', ctrl.createCat);
router.put('/:id', ctrl.updateCat);
router.delete('/:id', ctrl.deleteCat);

module.exports = router;
