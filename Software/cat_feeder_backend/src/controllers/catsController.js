'use strict';

/**
 * Cats Controller
 * All handlers return 501 until implemented.
 */

exports.getAllCats = (req, res) => {
    res.status(501).json({ status: 'not_implemented', message: 'GET /api/cats — coming soon' });
};

exports.getCatById = (req, res) => {
    res.status(501).json({ status: 'not_implemented', message: `GET /api/cats/${req.params.id} — coming soon` });
};

exports.createCat = (req, res) => {
    res.status(501).json({ status: 'not_implemented', message: 'POST /api/cats — coming soon' });
};

exports.updateCat = (req, res) => {
    res.status(501).json({ status: 'not_implemented', message: `PUT /api/cats/${req.params.id} — coming soon` });
};

exports.deleteCat = (req, res) => {
    res.status(501).json({ status: 'not_implemented', message: `DELETE /api/cats/${req.params.id} — coming soon` });
};
