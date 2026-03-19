import { useState, useEffect } from 'react'
import { supabase } from '../lib/supabase'
import { useAppStore } from '../store/store'

function CatModal({ cat, onClose, onSaved }) {
  const [form, setForm] = useState({
    name: cat?.name ?? '',
    breed: cat?.breed ?? '',
    date_of_birth: cat?.date_of_birth ?? '',
    weight_kg: cat?.weight_kg ?? '',
    diet_notes: cat?.diet_notes ?? '',
    photo_url: cat?.photo_url ?? '',
  })
  const [loading, setLoading] = useState(false)
  const [error, setError] = useState('')

  const set = (k, v) => setForm(f => ({ ...f, [k]: v }))

  const handleSave = async () => {
    if (!form.name.trim()) { setError('Name is required'); return }
    setLoading(true); setError('')
    try {
      // RLS policy requires owner_id = auth.uid() on insert
      const { data: { user } } = await supabase.auth.getUser()
      if (!user) throw new Error('Not authenticated')

      const payload = {
        owner_id: user.id,
        name: form.name.trim(),
        breed: form.breed || null,
        date_of_birth: form.date_of_birth || null,
        weight_kg: form.weight_kg ? parseFloat(form.weight_kg) : null,
        diet_notes: form.diet_notes || null,
        photo_url: form.photo_url || null,
      }
      if (cat) {
        // On update, don't change owner_id — just update the rest
        const { owner_id: _omit, ...updatePayload } = payload
        const { error: e } = await supabase.from('cats').update(updatePayload).eq('id', cat.id)
        if (e) throw e
      } else {
        const { error: e } = await supabase.from('cats').insert(payload)
        if (e) throw e
      }
      onSaved()
    } catch (e) {
      setError(e.message)
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="modal-overlay" onClick={e => e.target === e.currentTarget && onClose()}>
      <div className="modal">
        <div className="modal-header">
          <h3>{cat ? 'Edit Cat' : 'Add New Cat'} 🐱</h3>
          <button className="modal-close" onClick={onClose}>✕</button>
        </div>
        <div className="modal-body">
          {error && <div className="auth-error">{error}</div>}
          <div className="form-group">
            <label className="form-label">Name *</label>
            <input className="form-input" value={form.name} onChange={e=>set('name',e.target.value)} placeholder="e.g. Whiskers" />
          </div>
          <div className="form-group">
            <label className="form-label">Breed</label>
            <input className="form-input" value={form.breed} onChange={e=>set('breed',e.target.value)} placeholder="e.g. Persian, Maine Coon" />
          </div>
          <div style={{display:'grid',gridTemplateColumns:'1fr 1fr',gap:12}}>
            <div className="form-group">
              <label className="form-label">Date of Birth</label>
              <input type="date" className="form-input" value={form.date_of_birth} onChange={e=>set('date_of_birth',e.target.value)} />
            </div>
            <div className="form-group">
              <label className="form-label">Weight (kg)</label>
              <input type="number" className="form-input" value={form.weight_kg} onChange={e=>set('weight_kg',e.target.value)} placeholder="4.2" step="0.1" min="0" max="25" />
            </div>
          </div>
          <div className="form-group">
            <label className="form-label">Photo URL</label>
            <input className="form-input" value={form.photo_url} onChange={e=>set('photo_url',e.target.value)} placeholder="https://…" />
          </div>
          <div className="form-group">
            <label className="form-label">Diet Notes</label>
            <textarea className="form-textarea" value={form.diet_notes} onChange={e=>set('diet_notes',e.target.value)} placeholder="Veterinary notes, allergies, special requirements…" />
          </div>
        </div>
        <div className="modal-footer">
          <button className="btn btn-ghost" onClick={onClose}>Cancel</button>
          <button className="btn btn-primary" onClick={handleSave} disabled={loading}>
            {loading ? '◌ Saving…' : cat ? 'Save Changes' : 'Add Cat'}
          </button>
        </div>
      </div>
    </div>
  )
}

export default function CatsPage() {
  const { cats, fetchCats, addToast } = useAppStore()
  const [modal, setModal] = useState(null) // null | 'add' | cat object
  const [deleting, setDeleting] = useState(null)

  useEffect(() => { fetchCats() }, [])

  const handleDelete = async (cat) => {
    if (!window.confirm(`Archive ${cat.name}? This will hide them from your dashboard.`)) return
    setDeleting(cat.id)
    await supabase.from('cats').update({ archived: true }).eq('id', cat.id)
    await fetchCats()
    addToast(`${cat.name} archived`, 'info')
    setDeleting(null)
  }

  const handleSaved = async () => {
    setModal(null)
    await fetchCats()
    addToast('Cat profile saved! 🐾', 'success')
  }

  return (
    <div className="page-enter">
      <div className="page-header">
        <div>
          <h1>My Cats</h1>
          <p className="subtitle muted">Manage cat profiles and dietary information</p>
        </div>
        <button id="add-cat-btn" className="btn btn-primary" onClick={() => setModal('add')}>+ Add Cat</button>
      </div>

      {cats.length === 0 ? (
        <div className="card">
          <div className="empty-state">
            <div className="empty-icon">🐱</div>
            <h4>No cats registered yet</h4>
            <p>Add your first cat profile to start scheduling feeds</p>
            <button className="btn btn-primary" style={{marginTop:8}} onClick={() => setModal('add')}>Add first cat</button>
          </div>
        </div>
      ) : (
        <div className="card-grid card-grid-3">
          {cats.map(cat => (
            <div key={cat.id} className="cat-card page-enter">
              <div className="cat-card-photo">
                {cat.photo_url
                  ? <img src={cat.photo_url} alt={cat.name} style={{width:'100%',height:'100%',objectFit:'cover'}} />
                  : <span>🐱</span>
                }
              </div>
              <div className="cat-card-body">
                <div className="cat-card-name">{cat.name}</div>
                <div className="cat-card-info">
                  {cat.breed && <span>🏷 {cat.breed}</span>}
                  {cat.weight_kg && <span>⚖️ {cat.weight_kg} kg</span>}
                  {cat.date_of_birth && <span>🎂 {new Date(cat.date_of_birth).toLocaleDateString()}</span>}
                  {cat.diet_notes && <span style={{color:'var(--text-faint)',fontSize:'0.78rem'}}>{cat.diet_notes.substring(0,60)}{cat.diet_notes.length > 60 ? '…' : ''}</span>}
                </div>
              </div>
              <div className="cat-card-footer">
                <button className="btn btn-ghost btn-sm" onClick={() => setModal(cat)}>✏️ Edit</button>
                <button
                  className="btn btn-danger btn-sm"
                  onClick={() => handleDelete(cat)}
                  disabled={deleting === cat.id}
                >
                  {deleting === cat.id ? '◌' : '🗑 Archive'}
                </button>
              </div>
            </div>
          ))}
        </div>
      )}

      {(modal === 'add' || (modal && typeof modal === 'object')) && (
        <CatModal
          cat={modal === 'add' ? null : modal}
          onClose={() => setModal(null)}
          onSaved={handleSaved}
        />
      )}
    </div>
  )
}
