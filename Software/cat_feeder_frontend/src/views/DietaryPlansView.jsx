import { useState, useEffect, useCallback } from 'react';
import toast from 'react-hot-toast';
import TopBar from '../components/layout/TopBar';
import LoadingSpinner from '../components/ui/LoadingSpinner';
import { getAllPlans, createPlan, updatePlan, deletePlan, getSchedules, addSchedule } from '../services/api';

const NUM_SLOTS = 6;
const EMPTY_PLAN = { cat_id: '', daily_target_grams: '' };
const DEFAULT_SLOTS = Array.from({ length: NUM_SLOTS }, (_, i) => ({
    slot_index: i + 1,
    time: '',
    grams: '',
}));

export default function DietaryPlansView() {
    const [plans, setPlans] = useState([]);
    const [loading, setLoading] = useState(true);
    const [form, setForm] = useState(EMPTY_PLAN);
    const [slots, setSlots] = useState(DEFAULT_SLOTS);
    const [editId, setEditId] = useState(null);
    const [submitting, setSubmitting] = useState(false);

    const fetchPlans = useCallback(async () => {
        try {
            const data = await getAllPlans();
            setPlans(Array.isArray(data) ? data : data?.plans ?? []);
        } catch (err) {
            toast.error(`Load failed: ${err.message}`);
        } finally {
            setLoading(false);
        }
    }, []);

    useEffect(() => { fetchPlans(); }, [fetchPlans]);

    const handlePlanChange = (e) =>
        setForm((f) => ({ ...f, [e.target.name]: e.target.value }));

    const handleSlotChange = (index, field, value) =>
        setSlots((s) => s.map((sl) => sl.slot_index === index ? { ...sl, [field]: value } : sl));

    const handleSubmit = async (e) => {
        e.preventDefault();
        setSubmitting(true);
        try {
            let planId = editId;
            if (editId) {
                await updatePlan(editId, form);
                toast.success('Plan updated ✅');
            } else {
                const res = await createPlan(form);
                planId = res?.id ?? res?.plan?.id;
                toast.success('Plan created ✅');
            }
            // Save schedule slots
            if (planId) {
                const filled = slots.filter((s) => s.time && s.grams);
                await Promise.allSettled(filled.map((s) => addSchedule(planId, s)));
                if (filled.length) toast.success(`${filled.length} schedule slot(s) saved`);
            }
            setForm(EMPTY_PLAN);
            setSlots(DEFAULT_SLOTS);
            setEditId(null);
            fetchPlans();
        } catch (err) {
            toast.error(err.message);
        } finally {
            setSubmitting(false);
        }
    };

    const startEdit = async (plan) => {
        setEditId(plan.id);
        setForm({ cat_id: plan.cat_id ?? '', daily_target_grams: plan.daily_target_grams ?? '' });
        try {
            const scheds = await getSchedules(plan.id);
            const rows = Array.isArray(scheds) ? scheds : scheds?.schedules ?? [];
            const merged = DEFAULT_SLOTS.map((s) => {
                const found = rows.find((r) => r.slot_index === s.slot_index);
                return found ? { ...s, time: found.time ?? '', grams: found.grams ?? '' } : s;
            });
            setSlots(merged);
        } catch (_) { }
        window.scrollTo({ top: 0, behavior: 'smooth' });
    };

    const handleDelete = async (id) => {
        try {
            await deletePlan(id);
            toast.success('Plan deleted');
            setPlans((p) => p.filter((x) => x.id !== id));
        } catch (err) {
            toast.error(err.message);
        }
    };

    return (
        <div className="flex flex-col h-full">
            <TopBar title="Dietary Plans" subtitle="Configure daily feeding targets and time slots" />

            <div className="flex-1 overflow-y-auto p-6 space-y-6">
                {/* Plan form */}
                <div className="glass-card p-6 ring-1 ring-brand-500/10">
                    <p className="section-title mb-4">{editId ? '✏️ Edit Plan' : '➕ New Dietary Plan'}</p>
                    <form onSubmit={handleSubmit} className="space-y-4">
                        <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
                            <div>
                                <label className="label" htmlFor="dp-cat-id">Cat ID</label>
                                <input
                                    id="dp-cat-id" name="cat_id" type="number"
                                    placeholder="1" value={form.cat_id}
                                    onChange={handlePlanChange} className="input-field"
                                />
                            </div>
                            <div>
                                <label className="label" htmlFor="dp-target">Daily Target (g)</label>
                                <input
                                    id="dp-target" name="daily_target_grams" type="number"
                                    placeholder="200" value={form.daily_target_grams}
                                    onChange={handlePlanChange} className="input-field"
                                />
                            </div>
                        </div>

                        {/* Schedule slots */}
                        <div>
                            <p className="label mt-2">Feeding Schedule (up to {NUM_SLOTS} slots)</p>
                            <div className="grid grid-cols-2 sm:grid-cols-3 gap-3 mt-2">
                                {slots.map((sl) => (
                                    <div key={sl.slot_index} className="bg-surface-700 rounded-xl p-3 space-y-2">
                                        <p className="text-[11px] font-semibold text-gray-500 uppercase tracking-wider">Slot {sl.slot_index}</p>
                                        <input
                                            type="time"
                                            value={sl.time}
                                            onChange={(e) => handleSlotChange(sl.slot_index, 'time', e.target.value)}
                                            className="input-field text-sm py-1.5"
                                        />
                                        <input
                                            type="number"
                                            placeholder="Grams"
                                            value={sl.grams}
                                            onChange={(e) => handleSlotChange(sl.slot_index, 'grams', e.target.value)}
                                            className="input-field text-sm py-1.5"
                                        />
                                    </div>
                                ))}
                            </div>
                        </div>

                        <div className="flex gap-3">
                            <button type="submit" className="btn-primary" disabled={submitting}>
                                {submitting ? 'Saving…' : editId ? 'Update Plan' : 'Create Plan'}
                            </button>
                            {editId && (
                                <button type="button" className="btn-secondary"
                                    onClick={() => { setForm(EMPTY_PLAN); setSlots(DEFAULT_SLOTS); setEditId(null); }}>
                                    Cancel
                                </button>
                            )}
                        </div>
                    </form>
                </div>

                {/* Plans list */}
                <div>
                    <p className="section-title mb-3">Active Plans ({plans.length})</p>
                    {loading ? (
                        <LoadingSpinner className="py-16" />
                    ) : plans.length === 0 ? (
                        <div className="glass-card p-10 flex flex-col items-center gap-3 text-gray-600">
                            <span className="text-5xl">🍽️</span>
                            <p className="text-sm">No dietary plans yet. Create one above!</p>
                        </div>
                    ) : (
                        <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
                            {plans.map((plan) => (
                                <div key={plan.id} className="glass-card-hover p-5 animate-fade-in">
                                    <div className="flex items-start justify-between mb-2">
                                        <div className="flex items-center gap-2">
                                            <div className="w-8 h-8 rounded-xl bg-brand-500/10 flex items-center justify-center">🍽️</div>
                                            <p className="font-bold text-white text-sm">Plan #{plan.id}</p>
                                        </div>
                                        <div className="flex gap-1.5">
                                            <button id={`btn-edit-plan-${plan.id}`} onClick={() => startEdit(plan)}
                                                className="p-1.5 rounded-lg hover:bg-surface-600 text-gray-400 hover:text-gray-200 transition-colors">
                                                <svg xmlns="http://www.w3.org/2000/svg" width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round">
                                                    <path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7" /><path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z" />
                                                </svg>
                                            </button>
                                            <button id={`btn-delete-plan-${plan.id}`} onClick={() => handleDelete(plan.id)}
                                                className="p-1.5 rounded-lg hover:bg-red-500/10 text-gray-500 hover:text-red-400 transition-colors">
                                                <svg xmlns="http://www.w3.org/2000/svg" width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round">
                                                    <polyline points="3 6 5 6 21 6" /><path d="M19 6l-1 14H6L5 6" /><path d="M10 11v6M14 11v6M9 6V4h6v2" />
                                                </svg>
                                            </button>
                                        </div>
                                    </div>
                                    <p className="text-xs text-gray-500">Cat ID: <span className="text-gray-400">{plan.cat_id}</span></p>
                                    <p className="text-xs text-gray-500 mt-0.5">Daily Target: <span className="text-brand-400 font-bold">{plan.daily_target_grams ?? '—'} g</span></p>
                                </div>
                            ))}
                        </div>
                    )}
                </div>
            </div>
        </div>
    );
}
