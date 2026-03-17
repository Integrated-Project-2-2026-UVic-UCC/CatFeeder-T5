import axios from 'axios';

// All requests are proxied by Vite → http://localhost:3000
const http = axios.create({
    baseURL: '/api',
    timeout: 10000,
    headers: { 'Content-Type': 'application/json' },
});

// ─── Response interceptor (normalize errors) ──────────────────────────────────
http.interceptors.response.use(
    (res) => res,
    (err) => {
        const msg =
            err.response?.data?.message ||
            err.response?.data?.error ||
            err.message ||
            'Network error';
        return Promise.reject(new Error(msg));
    },
);

// ─── Sensor Logs ──────────────────────────────────────────────────────────────
export const getSensorLogs = (limit = 100) =>
    http.get(`/sensor-logs?limit=${limit}`).then((r) => r.data);

// ─── Feed Command ─────────────────────────────────────────────────────────────
export const postFeed = () =>
    http.post('/feed').then((r) => r.data);

// ─── Cats ─────────────────────────────────────────────────────────────────────
export const getAllCats = () => http.get('/cats').then((r) => r.data);
export const getCatById = (id) => http.get(`/cats/${id}`).then((r) => r.data);
export const createCat = (data) => http.post('/cats', data).then((r) => r.data);
export const updateCat = (id, data) => http.put(`/cats/${id}`, data).then((r) => r.data);
export const deleteCat = (id) => http.delete(`/cats/${id}`).then((r) => r.data);

// ─── Dietary Plans ────────────────────────────────────────────────────────────
export const getAllPlans = () => http.get('/dietary-plans').then((r) => r.data);
export const getPlanById = (id) => http.get(`/dietary-plans/${id}`).then((r) => r.data);
export const createPlan = (data) => http.post('/dietary-plans', data).then((r) => r.data);
export const updatePlan = (id, data) => http.put(`/dietary-plans/${id}`, data).then((r) => r.data);
export const deletePlan = (id) => http.delete(`/dietary-plans/${id}`).then((r) => r.data);

// ─── Schedules (nested under dietary plans) ───────────────────────────────────
export const getSchedules = (planId) =>
    http.get(`/dietary-plans/${planId}/schedules`).then((r) => r.data);
export const addSchedule = (planId, data) =>
    http.post(`/dietary-plans/${planId}/schedules`, data).then((r) => r.data);

// ─── Hardware Settings ────────────────────────────────────────────────────────
export const getHardwareSettings = () => http.get('/hardware-settings').then((r) => r.data);
export const updateHardwareSettings = (data) => http.put('/hardware-settings', data).then((r) => r.data);
