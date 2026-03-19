import { useState, useEffect } from 'react'
import { supabase } from '../lib/supabase'
import { useAppStore } from '../store/store'
import {
  BarChart, Bar, XAxis, YAxis, Tooltip, ResponsiveContainer, CartesianGrid, LineChart, Line
} from 'recharts'

const CustomTooltip = ({ active, payload, label }) => {
  if (!active || !payload?.length) return null
  return (
    <div style={{background:'var(--surface-2)',border:'1px solid var(--border)',borderRadius:8,padding:'8px 12px',fontSize:'0.82rem'}}>
      <div style={{color:'var(--text-muted)',marginBottom:2}}>{label}</div>
      {payload.map((p,i)=>(
        <div key={i} style={{color:'var(--accent)',fontWeight:600}}>{p.name}: {p.value}{typeof p.value==='number'&&p.name.includes('g')?' g':''}</div>
      ))}
    </div>
  )
}

export default function HistoryPage() {
  const { cats } = useAppStore()
  const [events, setEvents] = useState([])
  const [loading, setLoading] = useState(true)
  const [filterCat, setFilterCat] = useState('')
  const [filterRange, setFilterRange] = useState('7')
  const [page, setPage] = useState(0)
  const PAGE_SIZE = 10

  useEffect(() => { loadEvents() }, [filterCat, filterRange])

  const loadEvents = async () => {
    setLoading(true)
    const since = new Date()
    since.setDate(since.getDate() - parseInt(filterRange))

    let q = supabase.from('feed_events').select('*, cats(name)').gte('started_at', since.toISOString()).order('started_at', { ascending: false })
    if (filterCat) q = q.eq('cat_id', filterCat)

    const { data } = await q
    setEvents(data ?? [])
    setPage(0)
    setLoading(false)
  }

  const exportCSV = () => {
    const header = 'Cat,Time,Target (g),Actual (g),Type,Status\n'
    const rows = events.map(e =>
      `${e.cats?.name??''},${e.started_at},${e.target_grams??''},${e.actual_grams??''},${e.trigger_type},${e.status}`
    ).join('\n')
    const blob = new Blob([header + rows], { type: 'text/csv' })
    const a = document.createElement('a'); a.href = URL.createObjectURL(blob); a.download = 'feed_history.csv'; a.click()
  }

  // Chart data: daily totals
  const chartData = (() => {
    const map = {}
    events.forEach(e => {
      const day = new Date(e.started_at).toLocaleDateString('en', {month:'short',day:'numeric'})
      if (!map[day]) map[day] = { day, total: 0, feeds: 0 }
      map[day].total += e.actual_grams ?? 0
      map[day].feeds += 1
    })
    return Object.values(map).reverse()
  })()

  const paged = events.slice(page * PAGE_SIZE, (page+1) * PAGE_SIZE)
  const totalPages = Math.ceil(events.length / PAGE_SIZE)

  const avgAccuracy = events.filter(e=>e.target_grams&&e.actual_grams).reduce((acc,e)=>{
    return acc + (Math.min(e.actual_grams,e.target_grams)/e.target_grams*100)
  }, 0) / (events.filter(e=>e.target_grams&&e.actual_grams).length || 1)

  return (
    <div className="page-enter">
      <div className="page-header">
        <div>
          <h1>History & Analytics</h1>
          <p className="subtitle muted">Feed log and performance statistics</p>
        </div>
        <button className="btn btn-secondary" onClick={exportCSV}>⬇ Export CSV</button>
      </div>

      {/* Summary stats */}
      <div className="card-grid card-grid-3" style={{marginBottom:24}}>
        <div className="stat-card">
          <div className="stat-label">Total feeds</div>
          <div className="stat-value">{events.length}</div>
          <div className="stat-sub">Last {filterRange} days</div>
        </div>
        <div className="stat-card">
          <div className="stat-label">Total food dispensed</div>
          <div className="stat-value">{(events.reduce((a,e)=>a+(e.actual_grams??0),0)).toFixed(0)}g</div>
        </div>
        <div className="stat-card">
          <div className="stat-label">Avg. accuracy</div>
          <div className="stat-value" style={{color:'var(--accent)'}}>{avgAccuracy.toFixed(1)}%</div>
          <div className="stat-sub">Target vs actual</div>
        </div>
      </div>

      {/* Charts */}
      <div className="card-grid card-grid-2" style={{marginBottom:24}}>
        <div className="card">
          <div className="section-header"><h3>Daily Food Intake (g)</h3></div>
          <ResponsiveContainer width="100%" height={200}>
            <BarChart data={chartData} margin={{top:4,right:4,left:-20,bottom:0}}>
              <CartesianGrid strokeDasharray="3 3" stroke="var(--border)" />
              <XAxis dataKey="day" tick={{fill:'var(--text-muted)',fontSize:11}} axisLine={false} tickLine={false} />
              <YAxis tick={{fill:'var(--text-muted)',fontSize:11}} axisLine={false} tickLine={false} />
              <Tooltip content={<CustomTooltip />} />
              <Bar dataKey="total" name="Food (g)" fill="var(--accent)" radius={[4,4,0,0]} />
            </BarChart>
          </ResponsiveContainer>
        </div>
        <div className="card">
          <div className="section-header"><h3>Feeds per Day</h3></div>
          <ResponsiveContainer width="100%" height={200}>
            <LineChart data={chartData} margin={{top:4,right:4,left:-20,bottom:0}}>
              <CartesianGrid strokeDasharray="3 3" stroke="var(--border)" />
              <XAxis dataKey="day" tick={{fill:'var(--text-muted)',fontSize:11}} axisLine={false} tickLine={false} />
              <YAxis tick={{fill:'var(--text-muted)',fontSize:11}} axisLine={false} tickLine={false} />
              <Tooltip content={<CustomTooltip />} />
              <Line type="monotone" dataKey="feeds" name="Feeds" stroke="var(--accent)" strokeWidth={2} dot={{fill:'var(--accent)',r:3}} />
            </LineChart>
          </ResponsiveContainer>
        </div>
      </div>

      {/* Filters */}
      <div style={{display:'flex',gap:10,marginBottom:16,flexWrap:'wrap'}}>
        <select className="form-select" style={{width:'auto'}} value={filterCat} onChange={e=>setFilterCat(e.target.value)}>
          <option value="">All cats</option>
          {cats.map(c=><option key={c.id} value={c.id}>{c.name}</option>)}
        </select>
        <select className="form-select" style={{width:'auto'}} value={filterRange} onChange={e=>setFilterRange(e.target.value)}>
          <option value="7">Last 7 days</option>
          <option value="30">Last 30 days</option>
          <option value="90">Last 90 days</option>
        </select>
      </div>

      {/* Table */}
      <div className="card">
        {loading ? (
          <div style={{display:'flex',flexDirection:'column',gap:10,padding:4}}>
            {[...Array(5)].map((_,i)=><div key={i} className="skeleton" style={{height:44,borderRadius:8}}/>)}
          </div>
        ) : paged.length === 0 ? (
          <div className="empty-state">
            <div className="empty-icon">📋</div>
            <p>No feed events in this period</p>
          </div>
        ) : (
          <>
            <div className="table-wrap">
              <table>
                <thead>
                  <tr>
                    <th>Cat</th><th>Time</th><th>Target</th><th>Actual</th><th>Accuracy</th><th>Type</th><th>Status</th>
                  </tr>
                </thead>
                <tbody>
                  {paged.map(ev => {
                    const acc = ev.target_grams && ev.actual_grams ? Math.min(100,(ev.actual_grams/ev.target_grams*100)).toFixed(1) : null
                    return (
                      <tr key={ev.id}>
                        <td style={{fontWeight:600}}>{ev.cats?.name ?? '—'}</td>
                        <td style={{color:'var(--text-muted)',fontSize:'0.82rem'}}>{new Date(ev.started_at).toLocaleString()}</td>
                        <td>{ev.target_grams ? `${ev.target_grams}g` : '—'}</td>
                        <td style={{fontWeight:600}}>{ev.actual_grams ? `${ev.actual_grams}g` : '—'}</td>
                        <td>
                          {acc !== null && (
                            <span style={{color:acc >= 90 ? 'var(--green)' : acc >= 70 ? 'var(--amber)' : 'var(--red)',fontWeight:600}}>
                              {acc}%
                            </span>
                          )}
                        </td>
                        <td><span className={`badge ${ev.trigger_type==='manual'?'badge-accent':'badge-neutral'}`}>{ev.trigger_type}</span></td>
                        <td><span className={`badge ${ev.status==='completed'?'badge-online':ev.status==='error'?'badge-offline':'badge-amber'}`}>{ev.status}</span></td>
                      </tr>
                    )
                  })}
                </tbody>
              </table>
            </div>
            {totalPages > 1 && (
              <div style={{display:'flex',alignItems:'center',justifyContent:'center',gap:12,padding:'16px 0 4px'}}>
                <button className="btn btn-ghost btn-sm" onClick={()=>setPage(p=>Math.max(0,p-1))} disabled={page===0}>← Prev</button>
                <span style={{fontSize:'0.84rem',color:'var(--text-muted)'}}>Page {page+1} of {totalPages}</span>
                <button className="btn btn-ghost btn-sm" onClick={()=>setPage(p=>Math.min(totalPages-1,p+1))} disabled={page===totalPages-1}>Next →</button>
              </div>
            )}
          </>
        )}
      </div>
    </div>
  )
}
