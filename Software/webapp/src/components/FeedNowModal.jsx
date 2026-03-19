import { useState } from 'react'
import { supabase } from '../lib/supabase'

export default function FeedNowModal({ cats, device, onClose, onFed }) {
  const [catId, setCatId] = useState(cats[0]?.id ?? '')
  const [portion, setPortion] = useState(50)
  const [loading, setLoading] = useState(false)
  const [error, setError] = useState('')

  const handleFeed = async () => {
    if (!device) { setError('No device registered. Go to Device page first.'); return }
    if (!catId) { setError('Select a cat first.'); return }
    setLoading(true); setError('')
    try {
      const { data, error: err } = await supabase.from('commands').insert({
        device_id: device.id,
        cat_id: catId,
        command_type: 'feed',
        portion_grams: portion,
        status: 'pending'
      }).select().single()
      if (err) throw err
      onFed(data)
    } catch (e) {
      setError(e.message)
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="modal-overlay" onClick={e => { if (e.target === e.currentTarget) onClose() }}>
      <div className="modal">
        <div className="modal-header">
          <h3>⚡ Feed Now</h3>
          <button className="modal-close" onClick={onClose}>✕</button>
        </div>
        <div className="modal-body">
          {error && <div className="auth-error">{error}</div>}
          <div className="form-group">
            <label className="form-label">Select Cat</label>
            <select className="form-select" value={catId} onChange={e => setCatId(e.target.value)}>
              {cats.map(c => <option key={c.id} value={c.id}>{c.name}</option>)}
            </select>
          </div>
          <div className="form-group">
            <label className="form-label">Portion Size: <strong>{portion}g</strong></label>
            <input
              type="range"
              min={5} max={500} step={5}
              value={portion}
              onChange={e => setPortion(Number(e.target.value))}
              style={{width:'100%',accentColor:'var(--accent)'}}
            />
            <div style={{display:'flex',justifyContent:'space-between',fontSize:'0.75rem',color:'var(--text-faint)'}}>
              <span>5g</span><span>500g</span>
            </div>
          </div>
          <div className="card" style={{padding:'14px 16px'}}>
            <div style={{fontSize:'0.82rem',color:'var(--text-muted)',marginBottom:4}}>Feed summary</div>
            <div style={{fontFamily:'var(--font-display)',fontSize:'1.1rem'}}>
              {cats.find(c=>c.id===catId)?.name ?? '—'} → <strong style={{color:'var(--accent)'}}>{portion}g</strong>
            </div>
          </div>
        </div>
        <div className="modal-footer">
          <button className="btn btn-ghost" onClick={onClose}>Cancel</button>
          <button id="confirm-feed-btn" className="btn btn-primary" onClick={handleFeed} disabled={loading}>
            {loading ? '◌ Sending…' : '⚡ Confirm Feed'}
          </button>
        </div>
      </div>
    </div>
  )
}
