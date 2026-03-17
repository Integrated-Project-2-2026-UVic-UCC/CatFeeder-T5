import { useState, useEffect, useCallback } from 'react';
import toast from 'react-hot-toast';
import TopBar from '../components/layout/TopBar';
import LoadingSpinner from '../components/ui/LoadingSpinner';
import { getAllCats, createCat, updateCat, deleteCat } from '../services/api';

const EMPTY_FORM = { name: '', age: '', weight_kg: '', breed: '', rfid_tag_id: '' };

export default function CatProfilesView() {
    const [cats, setCats] = useState([]);
    const [loading, setLoading] = useState(true);
    const [form, setForm] = useState(EMPTY_FORM);
    const [editId, setEditId] = useState(null);
    const [submitting, setSubmitting] = useState(false);
    const [deleteId, setDeleteId] = useState(null); // confirm modal

    const fetchCats = useCallback(async () => {
        try {
            const data = await getAllCats();
            setCats(Array.isArray(data) ? data : data?.cats ?? []);
        } catch (err) {
            toast.error(`Load failed: ${err.message}`);
        } finally {
            setLoading(false);
        }
    }, []);

    useEffect(() => { fetchCats(); }, [fetchCats]);

    const handleChange = (e) =>
        setForm((f) => ({ ...f, [e.target.name]: e.target.value }));

    const handleSubmit = async (e) => {
        e.preventDefault();
        if (!form.name.trim()) return toast.error('Name is required');
        setSubmitting(true);
        try {
            if (editId) {
                await updateCat(editId, form);
                toast.success('Cat profile updated ✅');
            } else {
                await createCat(form);
                toast.success('Cat profile created 🐱');
            }
            setForm(EMPTY_FORM);
            setEditId(null);
            fetchCats();
        } catch (err) {
            toast.error(err.message);
        } finally {
            setSubmitting(false);
        }
    };

    const startEdit = (cat) => {
        setEditId(cat.id);
        setForm({
            name: cat.name ?? '',
            age: cat.age ?? '',
            weight_kg: cat.weight_kg ?? '',
            breed: cat.breed ?? '',
            rfid_tag_id: cat.rfid_tag_id ?? '',
        });
        window.scrollTo({ top: 0, behavior: 'smooth' });
    };

    const confirmDelete = async () => {
        if (!deleteId) return;
        try {
            await deleteCat(deleteId);
            toast.success('Cat profile deleted');
            setCats((c) => c.filter((x) => x.id !== deleteId));
        } catch (err) {
            toast.error(err.message);
        } finally {
            setDeleteId(null);
        }
    };

    return (
        <div className="flex flex-col h-full">
            <TopBar title="Cat Profiles" subtitle="Manage your cat's identity & RFID tags" />

            <div className="flex-1 overflow-y-auto p-6 space-y-6">
                {/* Form */}
                <div className="glass-card p-6 ring-1 ring-brand-500/10">
                    <p className="section-title mb-4">{editId ? '✏️ Edit Profile' : '➕ Add New Cat'}</p>
                    <form onSubmit={handleSubmit} className="grid grid-cols-1 sm:grid-cols-2 gap-4">
                        {[
                            { name: 'name', label: 'Name', placeholder: 'Whiskers', type: 'text' },
                            { name: 'age', label: 'Age (years)', placeholder: '3', type: 'number' },
                            { name: 'weight_kg', label: 'Weight (kg)', placeholder: '4.5', type: 'number' },
                            { name: 'breed', label: 'Breed', placeholder: 'Siamese', type: 'text' },
                            { name: 'rfid_tag_id', label: 'RFID Tag ID', placeholder: 'AA-BB-CC', type: 'text' },
                        ].map(({ name, label, placeholder, type }) => (
                            <div key={name}>
                                <label className="label" htmlFor={`cat-${name}`}>{label}</label>
                                <input
                                    id={`cat-${name}`}
                                    name={name}
                                    type={type}
                                    placeholder={placeholder}
                                    value={form[name]}
                                    onChange={handleChange}
                                    className="input-field"
                                    step={type === 'number' ? '0.1' : undefined}
                                />
                            </div>
                        ))}
                        <div className="sm:col-span-2 flex gap-3 pt-1">
                            <button type="submit" className="btn-primary" disabled={submitting}>
                                {submitting ? 'Saving…' : editId ? 'Update Profile' : 'Create Profile'}
                            </button>
                            {editId && (
                                <button
                                    type="button"
                                    className="btn-secondary"
                                    onClick={() => { setForm(EMPTY_FORM); setEditId(null); }}
                                >
                                    Cancel
                                </button>
                            )}
                        </div>
                    </form>
                </div>

                {/* Cat list */}
                <div>
                    <p className="section-title mb-3">All Cats ({cats.length})</p>
                    {loading ? (
                        <LoadingSpinner className="py-16" />
                    ) : cats.length === 0 ? (
                        <div className="glass-card p-10 flex flex-col items-center gap-3 text-gray-600">
                            <span className="text-5xl">🐱</span>
                            <p className="text-sm">No cat profiles yet. Add one above!</p>
                        </div>
                    ) : (
                        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4">
                            {cats.map((cat) => (
                                <div key={cat.id} className="glass-card-hover p-5 animate-fade-in">
                                    <div className="flex items-start justify-between mb-3">
                                        <div className="w-9 h-9 rounded-xl bg-brand-500/10 flex items-center justify-center text-xl">🐈</div>
                                        <div className="flex gap-1.5">
                                            <button
                                                id={`btn-edit-cat-${cat.id}`}
                                                onClick={() => startEdit(cat)}
                                                title="Edit"
                                                className="p-1.5 rounded-lg hover:bg-surface-600 text-gray-400 hover:text-gray-200 transition-colors"
                                            >
                                                <svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round">
                                                    <path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7" />
                                                    <path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z" />
                                                </svg>
                                            </button>
                                            <button
                                                id={`btn-delete-cat-${cat.id}`}
                                                onClick={() => setDeleteId(cat.id)}
                                                title="Delete"
                                                className="p-1.5 rounded-lg hover:bg-red-500/10 text-gray-500 hover:text-red-400 transition-colors"
                                            >
                                                <svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round">
                                                    <polyline points="3 6 5 6 21 6" /><path d="M19 6l-1 14H6L5 6" />
                                                    <path d="M10 11v6M14 11v6M9 6V4h6v2" />
                                                </svg>
                                            </button>
                                        </div>
                                    </div>
                                    <p className="font-bold text-white">{cat.name}</p>
                                    <div className="mt-2 space-y-1">
                                        {cat.breed && <p className="text-xs text-gray-500">Breed: <span className="text-gray-400">{cat.breed}</span></p>}
                                        {cat.age && <p className="text-xs text-gray-500">Age: <span className="text-gray-400">{cat.age} yrs</span></p>}
                                        {cat.weight_kg && <p className="text-xs text-gray-500">Weight: <span className="text-gray-400">{cat.weight_kg} kg</span></p>}
                                        {cat.rfid_tag_id && (
                                            <p className="text-xs text-gray-500">RFID: <span className="text-brand-400 font-mono">{cat.rfid_tag_id}</span></p>
                                        )}
                                    </div>
                                </div>
                            ))}
                        </div>
                    )}
                </div>
            </div>

            {/* Delete confirm modal */}
            {deleteId && (
                <div className="fixed inset-0 bg-black/60 backdrop-blur-sm flex items-center justify-center z-50 p-4">
                    <div className="glass-card ring-1 ring-red-500/20 p-6 max-w-sm w-full animate-slide-up">
                        <p className="font-bold text-white mb-1">Delete Cat Profile?</p>
                        <p className="text-sm text-gray-400 mb-5">This action cannot be undone.</p>
                        <div className="flex gap-3">
                            <button className="btn-danger flex-1" onClick={confirmDelete}>Delete</button>
                            <button className="btn-secondary flex-1" onClick={() => setDeleteId(null)}>Cancel</button>
                        </div>
                    </div>
                </div>
            )}
        </div>
    );
}
