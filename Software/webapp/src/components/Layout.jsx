import { Outlet, NavLink, useNavigate } from 'react-router-dom'
import { useEffect } from 'react'
import { useAuthStore, useAppStore } from '../store/store'

const NAV = [
  { icon: '⬛', label: 'Dashboard', to: '/' },
  { icon: '🐱', label: 'My Cats',   to: '/cats' },
  { icon: '🗓', label: 'Schedules', to: '/schedules' },
  { icon: '📊', label: 'History',   to: '/history' },
  { icon: '📡', label: 'Device',    to: '/device' },
  { icon: '💬', label: 'AI Chat',   to: '/chat' },
  { icon: '⚙️', label: 'Settings',  to: '/settings' },
]

// Better icons using unicode symbols
const ICONS = {
  '/':          '▦',
  '/cats':      '◉',
  '/schedules': '◷',
  '/history':   '◈',
  '/device':    '◎',
  '/chat':      '💬',
  '/settings':  '◌',
}

export default function Layout() {
  const { user, signOut } = useAuthStore()
  const { fetchCats, fetchDevices, fetchSchedules, fetchNotifications, notifications } = useAppStore()
  const navigate = useNavigate()

  useEffect(() => {
    fetchCats()
    fetchDevices()
    fetchSchedules()
    fetchNotifications()
  }, [])

  const handleSignOut = async () => {
    await signOut()
    navigate('/auth')
  }

  const initials = user?.email?.substring(0,2).toUpperCase() ?? '??'
  const unread = notifications.length

  return (
    <div className="app-shell">
      <aside className="sidebar">
        <div className="sidebar-logo">
          <div className="sidebar-logo-icon">🐾</div>
          <div className="sidebar-logo-text">Cat<span>Feeder</span></div>
        </div>

        <nav className="sidebar-nav">
          {[
            { icon: '▦', label: 'Dashboard', to: '/' },
            { icon: '◉', label: 'My Cats',   to: '/cats' },
            { icon: '◷', label: 'Schedules', to: '/schedules' },
            { icon: '◈', label: 'History',   to: '/history' },
            { icon: '◎', label: 'Device',    to: '/device' },
            { icon: '💬', label: 'AI Chat',  to: '/chat' },
          ].map(({ icon, label, to }) => (
            <NavLink
              key={to}
              to={to}
              end={to === '/'}
              className={({ isActive }) => `nav-item${isActive ? ' active' : ''}`}
            >
              <span className="nav-icon">{icon}</span>
              {label}
            </NavLink>
          ))}
        </nav>

        <div className="sidebar-bottom">
          <NavLink to="/settings" className={({ isActive }) => `nav-item${isActive ? ' active' : ''}`}>
            <span className="nav-icon">◌</span>
            Settings
          </NavLink>
        </div>
      </aside>

      <div className="main-content">
        <header className="header">
          <div className="header-left">
            <h2>CatFeeder T5</h2>
          </div>
          <div className="header-right">
            <button className="notif-btn" onClick={() => fetchNotifications()} title="Notifications">
              🔔
              {unread > 0 && <span className="notif-badge">{unread}</span>}
            </button>
            <div className="user-badge" onClick={handleSignOut} title="Click to sign out">
              <div className="user-avatar">{initials}</div>
              <span className="user-name">{user?.email?.split('@')[0]}</span>
            </div>
          </div>
        </header>

        <main className="page-content">
          <Outlet />
        </main>
      </div>
    </div>
  )
}
