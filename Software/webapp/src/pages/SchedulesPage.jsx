import { useState, useEffect } from 'react'
import { supabase } from '../lib/supabase'
import { useAppStore } from '../store/store'

const DAYS = ['Sun','Mon','Tue','Wed','Thu','Fri','Sat']

function ScheduleModal({ schedule, cats, devices, onClose, onSaved }) {
  const [form, setForm] = useState({
    cat_id: schedule?.cat_id ?? cats[0]?.id ?? '',
    device_id: schedule?.device_id ?? devices[0]?.id ?? '',
    time_of_day: schedule?.time_of_day?.substring(0,5) ?? '08:00',
    days_of_week: schedule?.days_of_week ?? [1,2,3,4,5],
    specific_date: schedule?.specific_date ?? '',
    portion_grams: schedule?.portion_grams ?? 50,
    enabled: schedule?.enabled ?? true,
    recurring: !schedule?.specific_date,
  })
  const [loading, setLoading] = useState(false)
  const [error, setError] = useState('')

  const setVal = (k,v) => setForm(f => ({...f, [k]:v}))
  const toggleDay = (d) => setVal('days_of_week', form.days_of_week.includes(d) ? form.days_of_week.filter(x=>x!==d) : [...form.days_of_week, d].sort())

  const handleSave = async () => {
    if (!form.cat_id) { setError('Select a cat'); return }
    if (form.portion_grams < 5 || form.portion_grams > 500) { setError('Portion must be 5–500g'); return }
    setLoading(true); setError('')
    try {
      const payload = {
        cat_id: form.cat_id,
        device_id: form.device_id || null,
        time_of_day: form.time_of_day,
        days_of_week: form.recurring ? form.days_of_week : null,
        specific_date: !form.recurring ? form.specific_date : null,
        portion_grams: form.portion_grams,
        enabled: form.enabled,
      }
      if (schedule) {
        const { error: e } = await supabase.from('schedules').update(payload).eq('id', schedule.id)
        if (e) throw e
      } else {
        const { error: e } = await supabase.from('schedules').insert(payload)
        if (e) throw e
      }
      onSaved()
    } catch(e) { setError(e.message) } finally { setLoading(false) }
  }

  return (
    <div className="modal-overlay" onClick={e => e.target === e.currentTarget && onClose()}>
      <div className="modal">
        <div className="modal-header">
          <h3>{schedule ? 'Edit Schedule' : 'New Schedule'}</h3>
          <button className="modal-close" onClick={onClose}>✕</button>
        </div>
        <div className="modal-body">
          {error && <div className="auth-error">{error}</div>}
          <div className="form-group">
            <label className="form-label">Cat</label>
            <select className="form-select" value={form.cat_id} onChange={e=>setVal('cat_id',e.target.value)}>
              {cats.map(c=><option key={c.id} value={c.id}>{c.name}</option>)}
            </select>
          </div>
          <div className="form-group">
            <label className="form-label">Feed Time</label>
            <input type="time" className="form-input" value={form.time_of_day} onChange={e=>setVal('time_of_day',e.target.value)} />
          </div>
          <div className="form-group">
            <label className="form-label">Portion: <strong>{form.portion_grams}g</strong></label>
            <input type="range" min={5} max={500} step={5} value={form.portion_grams} onChange={e=>setVal('portion_grams',Number(e.target.value))} style={{width:'100%',accentColor:'var(--accent)'}} />
          </div>
          <div className="form-group">
            <label className="form-label">Schedule Type</label>
            <div style={{display:'flex',gap:8}}>
              <button className={`btn btn-sm ${form.recurring ? 'btn-primary' : 'btn-ghost'}`} onClick={()=>setVal('recurring',true)}>Recurring</button>
              <button className={`btn btn-sm ${!form.recurring ? 'btn-primary' : 'btn-ghost'}`} onClick={()=>setVal('recurring',false)}>One-time</button>
            </div>
          </div>
          {form.recurring ? (
            <div className="form-group">
              <label className="form-label">Days of Week</label>
              <div style={{display:'flex',gap:6,flexWrap:'wrap'}}>
                {DAYS.map((d,i)=>(
                  <button key={i} onClick={()=>toggleDay(i)}
                    className={`btn btn-sm ${form.days_of_week.includes(i) ? 'btn-primary' : 'btn-ghost'}`}
                    style={{minWidth:44}}>
                    {d}
                  </button>
                ))}
              </div>
            </div>
          ) : (
            <div className="form-group">
              <label className="form-label">Date</label>
              <input type="date" className="form-input" value={form.specific_date} onChange={e=>setVal('specific_date',e.target.value)} />
            </div>
          )}
          <div style={{display:'flex',alignItems:'center',gap:12}}>
            <label className="form-label" style={{marginBottom:0}}>Enabled</label>
            <label className="toggle">
              <input type="checkbox" checked={form.enabled} onChange={e=>setVal('enabled',e.target.checked)} />
              <div className="toggle-track" />
              <div className="toggle-thumb" />
            </label>
          </div>
        </div>
        <div className="modal-footer">
          <button className="btn btn-ghost" onClick={onClose}>Cancel</button>
          <button className="btn btn-primary" onClick={handleSave} disabled={loading}>
            {loading ? '◌ Saving…' : schedule ? 'Save Changes' : 'Create Schedule'}
          </button>
        </div>
      </div>
    </div>
  )
}

export default function SchedulesPage() {
  const { cats, devices, schedules, fetchSchedules, addToast } = useAppStore()
  const [modal, setModal] = useState(null)
  const [view, setView] = useState('list') // 'list' | 'calendar'

  useEffect(() => { fetchSchedules() }, [])

  const toggleEnabled = async (s) => {
    await supabase.from('schedules').update({ enabled: !s.enabled }).eq('id', s.id)
    await fetchSchedules()
  }

  const deleteSchedule = async (s) => {
    if (!window.confirm('Delete this schedule?')) return
    await supabase.from('schedules').delete().eq('id', s.id)
    await fetchSchedules()
    addToast('Schedule deleted', 'info')
  }

  const handleSaved = async () => {
    setModal(null)
    await fetchSchedules()
    addToast('Schedule saved!', 'success')
  }

  // Group schedules by day for calendar view
  const calendarDays = DAYS.map((day, i) => ({
    day,
    i,
    schedules: schedules.filter(s => s.days_of_week?.includes(i))
  }))

  return (
    <div className="page-enter">
      <div className="page-header">
        <div>
          <h1>Schedules</h1>
          <p className="subtitle muted">Recurring and one-time feeding plans</p>
        </div>
        <div style={{display:'flex',gap:8}}>
          <button className={`btn btn-sm ${view==='list'?'btn-primary':'btn-ghost'}`} onClick={()=>setView('list')}>☰ List</button>
          <button className={`btn btn-sm ${view==='calendar'?'btn-primary':'btn-ghost'}`} onClick={()=>setView('calendar')}>▦ Week</button>
          <button className="btn btn-primary" id="new-schedule-btn" onClick={()=>setModal('new')}>+ New Schedule</button>
        </div>
      </div>

      {view === 'calendar' ? (
        <div style={{display:'grid',gridTemplateColumns:'repeat(7,1fr)',gap:8}}>
          {calendarDays.map(({ day, schedules: ds }) => (
            <div key={day} className="card" style={{minHeight:120}}>
              <div className="label" style={{marginBottom:8}}>{day}</div>
              {ds.length === 0
                ? <div style={{fontSize:'0.75rem',color:'var(--text-faint)'}}>—</div>
                : ds.map(s => (
                  <div key={s.id} style={{
                    background: s.enabled ? 'var(--accent-glow)' : 'var(--neutral)',
                    border: `1px solid ${s.enabled ? 'rgba(255,122,47,0.3)' : 'var(--border)'}`,
                    borderRadius:'var(--radius-sm)',
                    padding:'6px 8px',
                    marginBottom:4,
                    fontSize:'0.75rem',
                  }}>
                    <div style={{fontWeight:600,color:s.enabled?'var(--accent)':'var(--text-muted)'}}>{s.time_of_day?.substring(0,5)}</div>
                    <div style={{color:'var(--text-muted)'}}>{s.cats?.name} · {s.portion_grams}g</div>
                  </div>
                ))
              }
            </div>
          ))}
        </div>
      ) : (
        <div className="card">
          {schedules.length === 0 ? (
            <div className="empty-state">
              <div className="empty-icon">🗓</div>
              <h4>No schedules yet</h4>
              <p>Create a recurring or one-time feeding schedule</p>
              <button className="btn btn-primary" style={{marginTop:8}} onClick={()=>setModal('new')}>New Schedule</button>
            </div>
          ) : (
            <div className="table-wrap">
              <table>
                <thead>
                  <tr>
                    <th>Cat</th><th>Time</th><th>Days / Date</th><th>Portion</th><th>Enabled</th><th>Actions</th>
                  </tr>
                </thead>
                <tbody>
                  {schedules.map(s => (
                    <tr key={s.id}>
                      <td style={{fontWeight:600}}>{s.cats?.name ?? '—'}</td>
                      <td style={{fontFamily:'var(--font-display)',color:'var(--accent)'}}>{s.time_of_day?.substring(0,5)}</td>
                      <td style={{fontSize:'0.82rem',color:'var(--text-muted)'}}>
                        {s.days_of_week ? s.days_of_week.map(d=>DAYS[d]).join(', ') : s.specific_date ?? '—'}
                      </td>
                      <td><span className="badge badge-accent">{s.portion_grams}g</span></td>
                      <td>
                        <label className="toggle">
                          <input type="checkbox" checked={s.enabled} onChange={()=>toggleEnabled(s)} />
                          <div className="toggle-track"/><div className="toggle-thumb"/>
                        </label>
                      </td>
                      <td>
                        <div style={{display:'flex',gap:6}}>
                          <button className="btn btn-ghost btn-sm" onClick={()=>setModal(s)}>✏️</button>
                          <button className="btn btn-danger btn-sm" onClick={()=>deleteSchedule(s)}>🗑</button>
                        </div>
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}
        </div>
      )}

      {modal && (
        <ScheduleModal
          schedule={modal === 'new' ? null : modal}
          cats={cats}
          devices={devices}
          onClose={()=>setModal(null)}
          onSaved={handleSaved}
        />
      )}
    </div>
  )
}
