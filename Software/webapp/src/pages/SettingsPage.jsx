import { useState, useEffect } from 'react'
import { supabase } from '../lib/supabase'
import { useAppStore } from '../store/store'

export default function SettingsPage() {
  const { devices, fetchDevices, addToast } = useAppStore()
  const device = devices[0]
  const [config, setConfig] = useState(null)
  const [saving, setSaving] = useState(false)
  const [profile, setProfile] = useState({ display_name: '', email: '' })

  useEffect(() => {
    fetchDevices()
    loadConfig()
    loadProfile()
  }, [])

  const loadConfig = async () => {
    if (!device) return
    const { data } = await supabase.from('device_config').select('*').eq('device_id', device.id).single()
    setConfig(data)
  }

  const loadProfile = async () => {
    const { data } = await supabase.from('profiles').select('*').single()
    if (data) setProfile({ display_name: data.display_name ?? '', email: data.email ?? '' })
  }

  const saveConfig = async () => {
    if (!device || !config) return
    setSaving(true)
    const { error } = await supabase.from('device_config').upsert({
      ...config,
      device_id: device.id,
      config_version: (config.config_version ?? 0) + 1,
      updated_at: new Date().toISOString()
    })
    setSaving(false)
    if (error) addToast(error.message, 'error')
    else addToast('Settings saved and pushed to device!', 'success')
  }

  const sendTare = async () => {
    if (!device) return
    await supabase.from('commands').insert({ device_id: device.id, command_type: 'tare', status: 'pending' })
    addToast('Tare command sent!', 'success')
  }

  const setC = (k, v) => setConfig(c => ({ ...c, [k]: v }))

  if (!config && device) loadConfig()

  return (
    <div className="page-enter">
      <div className="page-header">
        <div>
          <h1>Settings & Calibration</h1>
          <p className="subtitle muted">User preferences, device calibration, and system configuration</p>
        </div>
        {config && (
          <button className="btn btn-primary" onClick={saveConfig} disabled={saving}>
            {saving ? '◌ Saving…' : '💾 Save All Changes'}
          </button>
        )}
      </div>

      <div style={{display:'flex',flexDirection:'column',gap:20}}>

        {/* Profile */}
        <div className="card">
          <h3 style={{marginBottom:16}}>User Profile</h3>
          <div style={{display:'grid',gridTemplateColumns:'1fr 1fr',gap:16}}>
            <div className="form-group">
              <label className="form-label">Display Name</label>
              <input className="form-input" value={profile.display_name} onChange={e=>setProfile(p=>({...p,display_name:e.target.value}))} />
            </div>
            <div className="form-group">
              <label className="form-label">Email</label>
              <input className="form-input" value={profile.email} disabled style={{opacity:0.5}} />
            </div>
          </div>
          <div style={{marginTop:16}}>
            <button className="btn btn-secondary" onClick={async()=>{
              await supabase.from('profiles').update({display_name:profile.display_name}).eq('id',(await supabase.auth.getUser()).data.user?.id)
              addToast('Profile updated!','success')
            }}>Save Profile</button>
          </div>
        </div>

        {/* Notifications */}
        <div className="card">
          <h3 style={{marginBottom:16}}>Notification Preferences</h3>
          <div style={{display:'flex',flexDirection:'column',gap:14}}>
            {[
              { k: 'notification_in_app', label: 'In-app notifications', desc: 'Show alerts in the CatFeeder dashboard' },
              { k: 'notification_email',  label: 'Email notifications',  desc: 'Receive missed feed and offline alerts by email' },
            ].map(({ k, label, desc }) => (
              <div key={k} style={{display:'flex',alignItems:'center',justifyContent:'space-between'}}>
                <div>
                  <div style={{fontWeight:500}}>{label}</div>
                  <div style={{fontSize:'0.82rem',color:'var(--text-muted)'}}>{desc}</div>
                </div>
                <label className="toggle">
                  <input type="checkbox" defaultChecked onChange={()=>{}} />
                  <div className="toggle-track"/><div className="toggle-thumb"/>
                </label>
              </div>
            ))}
          </div>
        </div>

        {/* Device calibration */}
        {device ? (
          <>
            <div className="card">
              <h3 style={{marginBottom:16}}>Scale Calibration (HX711)</h3>
              <div style={{display:'grid',gridTemplateColumns:'1fr 1fr',gap:16}}>
                <div className="form-group">
                  <label className="form-label">Calibration Factor</label>
                  <input
                    type="number"
                    className="form-input"
                    value={config?.calibration_factor ?? 1}
                    onChange={e=>setC('calibration_factor',parseFloat(e.target.value))}
                    step="0.001"
                    placeholder="e.g. 2280.9"
                  />
                  <div style={{fontSize:'0.75rem',color:'var(--text-faint)',marginTop:4}}>
                    Enter the units/gram ratio from your HX711 calibration
                  </div>
                </div>
                <div className="form-group">
                  <label className="form-label">Hopper Capacity (g)</label>
                  <input
                    type="number"
                    className="form-input"
                    value={config?.hopper_capacity_grams ?? 2000}
                    onChange={e=>setC('hopper_capacity_grams',parseFloat(e.target.value))}
                    placeholder="2000"
                  />
                </div>
              </div>
              <div style={{marginTop:14}}>
                <button className="btn btn-secondary" onClick={sendTare}>⚖️ Tare Scale (Zero)</button>
              </div>
            </div>

            <div className="card">
              <h3 style={{marginBottom:16}}>Motor Settings</h3>
              <div className="form-group" style={{maxWidth:280}}>
                <label className="form-label">Max Run Time: <strong>{config?.motor_timeout_seconds ?? 30}s</strong></label>
                <input
                  type="range" min={5} max={120} step={5}
                  value={config?.motor_timeout_seconds ?? 30}
                  onChange={e=>setC('motor_timeout_seconds',parseInt(e.target.value))}
                  style={{width:'100%',accentColor:'var(--accent)'}}
                />
                <div style={{display:'flex',justifyContent:'space-between',fontSize:'0.75rem',color:'var(--text-faint)'}}>
                  <span>5s</span><span>120s</span>
                </div>
                <div style={{fontSize:'0.75rem',color:'var(--text-faint)',marginTop:4}}>
                  Safety limit — motor stops after this time to prevent mechanical overrun
                </div>
              </div>
            </div>
          </>
        ) : (
          <div className="card">
            <div className="empty-state" style={{padding:'24px 20px'}}>
              <div className="empty-icon">📡</div>
              <p>Register a device first to access calibration settings</p>
            </div>
          </div>
        )}

        {/* Danger zone */}
        <div className="card" style={{borderColor:'rgba(248,113,113,0.2)'}}>
          <h3 style={{marginBottom:4}}>Danger Zone</h3>
          <p style={{fontSize:'0.84rem',color:'var(--text-muted)',marginBottom:16}}>Irreversible actions — proceed with caution</p>
          <button className="btn btn-danger" onClick={async()=>{
            if (window.confirm('Sign out of your account?')) {
              await supabase.auth.signOut()
              window.location.href = '/auth'
            }
          }}>
            Sign Out
          </button>
        </div>

      </div>
    </div>
  )
}
