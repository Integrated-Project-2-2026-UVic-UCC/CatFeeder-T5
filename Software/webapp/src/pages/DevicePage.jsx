import { useState, useEffect, useRef } from 'react'
import { supabase } from '../lib/supabase'
import { useAppStore } from '../store/store'

export default function DevicePage() {
  const { devices, fetchDevices, addToast } = useAppStore()
  const [liveWeight, setLiveWeight] = useState(null)
  const [loading, setLoading] = useState(false)
  const [editName, setEditName] = useState(false)
  const [name, setName] = useState('')
  const [showRegModal, setShowRegModal] = useState(false)
  const [regMac, setRegMac] = useState('')
  const [regName, setRegName] = useState('My CatFeeder')

  const device = devices[0]

  useEffect(() => {
    fetchDevices()

    // Realtime weight subscription
    const sub = supabase.channel('device-weight')
      .on('postgres_changes', { event: 'INSERT', schema: 'public', table: 'realtime_weight' }, p => {
        setLiveWeight(p.new.weight_grams)
      })
      .subscribe()

    return () => supabase.removeChannel(sub)
  }, [])

  useEffect(() => {
    if (device) setName(device.name ?? 'My CatFeeder')
  }, [device])

  const sendCommand = async (cmdType) => {
    if (!device) return
    if (!window.confirm(`Send "${cmdType}" command to device?`)) return
    setLoading(true)
    const { error } = await supabase.from('commands').insert({
      device_id: device.id,
      command_type: cmdType,
      status: 'pending'
    })
    setLoading(false)
    if (error) addToast(`Error: ${error.message}`, 'error')
    else addToast(`Command "${cmdType}" sent!`, 'success')
  }

  const saveName = async () => {
    if (!device || !name.trim()) return
    await supabase.from('devices').update({ name: name.trim() }).eq('id', device.id)
    await fetchDevices()
    setEditName(false)
    addToast('Device renamed', 'success')
  }

  const registerDevice = async () => {
    const { data: { user } } = await supabase.auth.getUser()
    if (!user) { addToast('Not authenticated', 'error'); return }
    const { error } = await supabase.from('devices').insert({
      owner_id: user.id,
      name: regName,
      device_mac: regMac || null
    })
    if (error) { addToast(error.message, 'error'); return }
    await fetchDevices()
    setShowRegModal(false)
    addToast('Device registered!', 'success')
  }

  const statusColor = device?.status === 'idle' || device?.status === 'dispensing'
    ? 'badge-online'
    : device?.status === 'fault_motor' || device?.status === 'fault_sensor'
    ? 'badge-offline' : 'badge-amber'

  const getStatusLabel = (s) => {
    const map = { idle:'Idle', dispensing:'Dispensing', fault_motor:'Motor Fault', fault_sensor:'Sensor Fault', offline:'Offline' }
    return map[s] ?? s ?? 'Unknown'
  }

  return (
    <div className="page-enter">
      <div className="page-header">
        <div>
          <h1>Device Management</h1>
          <p className="subtitle muted">ESP32 CatFeeder hardware status & control</p>
        </div>
        {!device && (
          <button className="btn btn-primary" onClick={() => setShowRegModal(true)}>+ Register Device</button>
        )}
      </div>

      {!device ? (
        <div className="card">
          <div className="empty-state">
            <div className="empty-icon">📡</div>
            <h4>No device registered</h4>
            <p>Register your ESP32 CatFeeder device to start monitoring</p>
            <button className="btn btn-primary" style={{marginTop:8}} onClick={() => setShowRegModal(true)}>Register Device</button>
          </div>
        </div>
      ) : (
        <div style={{display:'flex',flexDirection:'column',gap:20}}>

          {/* Status Card */}
          <div className="card">
            <div className="section-header">
              <h3>Device Status</h3>
              <span className={`badge ${statusColor}`}>
                <div className="status-dot" style={{animationDuration:'2s'}} />
                {getStatusLabel(device.status)}
              </span>
            </div>

            <div style={{display:'grid',gridTemplateColumns:'repeat(3,1fr)',gap:16,marginTop:4}}>
              <div>
                <div className="label">Device Name</div>
                {editName ? (
                  <div style={{display:'flex',gap:8,marginTop:4}}>
                    <input className="form-input" value={name} onChange={e=>setName(e.target.value)} style={{flex:1}} />
                    <button className="btn btn-primary btn-sm" onClick={saveName}>Save</button>
                    <button className="btn btn-ghost btn-sm" onClick={()=>setEditName(false)}>✕</button>
                  </div>
                ) : (
                  <div style={{display:'flex',alignItems:'center',gap:8,marginTop:4}}>
                    <span style={{fontFamily:'var(--font-display)',fontSize:'1rem'}}>{device.name}</span>
                    <button className="btn btn-ghost btn-sm" onClick={()=>setEditName(true)}>✏️</button>
                  </div>
                )}
              </div>
              <div>
                <div className="label">Last Heartbeat</div>
                <div style={{marginTop:4,fontFamily:'var(--font-display)'}}>
                  {device.last_seen ? new Date(device.last_seen).toLocaleString() : 'Never'}
                </div>
              </div>
              <div>
                <div className="label">Firmware</div>
                <div style={{marginTop:4,fontFamily:'var(--font-display)'}}>{device.firmware_version ?? 'Unknown'}</div>
              </div>
              <div>
                <div className="label">MAC Address</div>
                <div style={{marginTop:4,fontFamily:'monospace',fontSize:'0.85rem',color:'var(--text-muted)'}}>{device.device_mac ?? '—'}</div>
              </div>
              <div>
                <div className="label">Device ID</div>
                <div style={{marginTop:4,fontFamily:'monospace',fontSize:'0.75rem',color:'var(--text-faint)'}}>{device.id}</div>
              </div>
            </div>
          </div>

          {/* Live Sensor */}
          <div className="card">
            <div className="section-header">
              <h3>Live Sensor Readings</h3>
              {liveWeight !== null && <span className="badge badge-accent">◉ LIVE</span>}
            </div>
            <div style={{display:'flex',alignItems:'baseline',gap:12}}>
              <div className="display" style={{color:'var(--accent)'}}>
                {liveWeight !== null ? `${liveWeight}g` : '— g'}
              </div>
              <div style={{color:'var(--text-muted)',fontSize:'0.88rem'}}>current weight</div>
            </div>
            {liveWeight === null && (
              <div style={{fontSize:'0.82rem',color:'var(--text-faint)',marginTop:8}}>
                Waiting for ESP32 data… (updates when device is active)
              </div>
            )}
          </div>

          {/* Controls */}
          <div className="card">
            <h3 style={{marginBottom:16}}>Device Controls</h3>
            <div style={{display:'flex',gap:10,flexWrap:'wrap'}}>
              <button
                className="btn btn-secondary"
                onClick={() => sendCommand('tare')}
                disabled={loading}
                id="tare-btn"
              >
                ⚖️ Send Tare
              </button>
              <button
                className="btn btn-danger"
                onClick={() => sendCommand('restart')}
                disabled={loading}
                id="restart-btn"
              >
                🔄 Restart Device
              </button>
            </div>
            <p style={{marginTop:12,fontSize:'0.82rem',color:'var(--text-muted)'}}>
              Commands are queued in Supabase and executed on the next ESP32 poll cycle (max 60s delay).
            </p>
          </div>

        </div>
      )}

      {/* Register device modal */}
      {showRegModal && (
        <div className="modal-overlay" onClick={e=>e.target===e.currentTarget&&setShowRegModal(false)}>
          <div className="modal">
            <div className="modal-header">
              <h3>Register ESP32 Device</h3>
              <button className="modal-close" onClick={()=>setShowRegModal(false)}>✕</button>
            </div>
            <div className="modal-body">
              <div className="form-group">
                <label className="form-label">Device Name</label>
                <input className="form-input" value={regName} onChange={e=>setRegName(e.target.value)} />
              </div>
              <div className="form-group">
                <label className="form-label">MAC Address (optional)</label>
                <input className="form-input" value={regMac} onChange={e=>setRegMac(e.target.value)} placeholder="AA:BB:CC:DD:EE:FF" />
              </div>
            </div>
            <div className="modal-footer">
              <button className="btn btn-ghost" onClick={()=>setShowRegModal(false)}>Cancel</button>
              <button className="btn btn-primary" onClick={registerDevice}>Register</button>
            </div>
          </div>
        </div>
      )}
    </div>
  )
}
