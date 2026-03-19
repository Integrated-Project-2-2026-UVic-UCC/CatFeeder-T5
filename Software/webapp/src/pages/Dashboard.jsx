import { useState, useEffect } from 'react'
import { useAppStore } from '../store/store'
import { supabase } from '../lib/supabase'
import FeedNowModal from '../components/FeedNowModal'

function StatCard({ value, label, sub, accent }) {
  return (
    <div className={`stat-card${accent ? ' card accent' : ''}`}>
      <div className="stat-label">{label}</div>
      <div className="stat-value display">{value}</div>
      {sub && <div className="stat-sub">{sub}</div>}
    </div>
  )
}

function CatSummaryCard({ cat, onClick }) {
  const nextSchedule = null // simplified
  return (
    <div className="cat-card page-enter" onClick={() => onClick(cat)}>
      <div className="cat-card-photo">
        {cat.photo_url
          ? <img src={cat.photo_url} alt={cat.name} style={{width:'100%',height:'100%',objectFit:'cover'}} />
          : <span>🐱</span>
        }
      </div>
      <div className="cat-card-body">
        <div className="cat-card-name">{cat.name}</div>
        <div className="cat-card-info">
          {cat.breed && <span>{cat.breed}</span>}
          {cat.weight_kg && <span>⚖️ {cat.weight_kg} kg</span>}
        </div>
      </div>
      <div className="cat-card-footer">
        <span>🕐 Next feed pending</span>
        <span className="badge badge-online">Active</span>
      </div>
    </div>
  )
}

export default function Dashboard() {
  const { cats, devices, fetchCats, fetchDevices, addToast } = useAppStore()
  const [showFeedModal, setShowFeedModal] = useState(false)
  const [feedEvents, setFeedEvents] = useState([])
  const [activeFeed, setActiveFeed] = useState(null)
  const [liveWeight, setLiveWeight] = useState(null)

  useEffect(() => {
    fetchCats()
    fetchDevices()
    loadRecentEvents()

    // Subscribe to realtime_weight for active dispensing
    const sub = supabase
      .channel('weight')
      .on('postgres_changes', { event: 'INSERT', schema: 'public', table: 'realtime_weight' }, (payload) => {
        setLiveWeight(payload.new.weight_grams)
      })
      .subscribe()

    // Subscribe to commands for live status
    const cmdSub = supabase
      .channel('commands')
      .on('postgres_changes', { event: '*', schema: 'public', table: 'commands' }, (payload) => {
        if (payload.new?.status === 'completed') {
          addToast('Feed cycle completed! 🐾', 'success')
          setActiveFeed(null)
          loadRecentEvents()
        }
        if (payload.new?.status === 'error') {
          addToast('Feed error — check device', 'error')
          setActiveFeed(null)
        }
      })
      .subscribe()

    return () => { supabase.removeChannel(sub); supabase.removeChannel(cmdSub) }
  }, [])

  const loadRecentEvents = async () => {
    const { data } = await supabase
      .from('feed_events')
      .select('*, cats(name)')
      .order('started_at', { ascending: false })
      .limit(5)
    if (data) setFeedEvents(data)
  }

  const device = devices[0]
  const totalFeedsToday = feedEvents.filter(e => {
    const d = new Date(e.started_at)
    const today = new Date()
    return d.toDateString() === today.toDateString()
  }).length

  return (
    <div className="page-enter">
      {/* Live dispensing banner */}
      {activeFeed && (
        <div className="live-banner">
          <div className="live-dot" />
          <div style={{flex:1}}>
            <div style={{fontWeight:600,fontFamily:'var(--font-display)'}}>Dispensing in progress…</div>
            <div style={{fontSize:'0.82rem',color:'var(--text-muted)',marginTop:2}}>
              {liveWeight != null ? `${liveWeight}g / ${activeFeed.portion_grams}g` : 'Waiting for weight data…'}
            </div>
            {liveWeight != null && activeFeed.portion_grams && (
              <div className="progress-bar" style={{marginTop:8}}>
                <div className="progress-fill" style={{width:`${Math.min(100,(liveWeight/activeFeed.portion_grams)*100)}%`}} />
              </div>
            )}
          </div>
          <span className="badge badge-accent">◉ LIVE</span>
        </div>
      )}

      <div className="page-header">
        <div>
          <h1>Dashboard</h1>
          <p className="subtitle muted">Overview of your feeding system</p>
        </div>
        <button className="btn btn-primary" onClick={() => setShowFeedModal(true)} id="feed-now-btn">
          ⚡ Feed Now
        </button>
      </div>

      {/* Stats Row */}
      <div className="card-grid card-grid-4" style={{marginBottom:24}}>
        <StatCard value={cats.length} label="Cats Registered" sub="Active profiles" accent />
        <StatCard value={totalFeedsToday} label="Feeds Today" sub="All cats" />
        <StatCard value={device ? (device.status === 'idle' || device.status === 'dispensing' ? '●' : '○') : '—'} label="Device Status" sub={device?.status ?? 'No device'} />
        <StatCard value={feedEvents[0]?.actual_grams ? `${feedEvents[0].actual_grams}g` : '—'} label="Last Portion" sub={feedEvents[0]?.cats?.name ?? 'N/A'} />
      </div>

      {/* Cat Cards */}
      <div className="section-header">
        <h3>Your Cats</h3>
        <a href="/cats" style={{color:'var(--accent)',fontSize:'0.84rem'}}>Manage all →</a>
      </div>

      {cats.length === 0 ? (
        <div className="card">
          <div className="empty-state">
            <div className="empty-icon">🐱</div>
            <h4>No cats yet</h4>
            <p>Head to My Cats to create your first cat profile</p>
          </div>
        </div>
      ) : (
        <div className="card-grid card-grid-3" style={{marginBottom:28}}>
          {cats.map(cat => (
            <CatSummaryCard key={cat.id} cat={cat} onClick={() => {}} />
          ))}
        </div>
      )}

      {/* Recent Feed Events */}
      <div className="section-header">
        <h3>Recent Feed Events</h3>
      </div>
      <div className="card">
        {feedEvents.length === 0 ? (
          <div className="empty-state" style={{padding:'28px 20px'}}>
            <div className="empty-icon">📋</div>
            <p>No feed events yet</p>
          </div>
        ) : (
          <div className="table-wrap">
            <table>
              <thead>
                <tr>
                  <th>Cat</th>
                  <th>Time</th>
                  <th>Target</th>
                  <th>Actual</th>
                  <th>Type</th>
                  <th>Status</th>
                </tr>
              </thead>
              <tbody>
                {feedEvents.map(ev => (
                  <tr key={ev.id}>
                    <td>{ev.cats?.name ?? '—'}</td>
                    <td style={{color:'var(--text-muted)'}}>{new Date(ev.started_at).toLocaleString()}</td>
                    <td>{ev.target_grams ? `${ev.target_grams}g` : '—'}</td>
                    <td style={{fontWeight:600}}>{ev.actual_grams ? `${ev.actual_grams}g` : '—'}</td>
                    <td>
                      <span className={`badge ${ev.trigger_type === 'manual' ? 'badge-accent' : 'badge-neutral'}`}>
                        {ev.trigger_type}
                      </span>
                    </td>
                    <td>
                      <span className={`badge ${ev.status === 'completed' ? 'badge-online' : ev.status === 'error' ? 'badge-offline' : 'badge-amber'}`}>
                        {ev.status}
                      </span>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        )}
      </div>

      {/* FAB */}
      <button className="fab" onClick={() => setShowFeedModal(true)} title="Feed Now">⚡</button>

      {showFeedModal && (
        <FeedNowModal
          cats={cats}
          device={device}
          onClose={() => setShowFeedModal(false)}
          onFed={(cmd) => {
            setActiveFeed(cmd)
            setShowFeedModal(false)
            addToast('Feed command sent! 🐾', 'success')
          }}
        />
      )}
    </div>
  )
}
