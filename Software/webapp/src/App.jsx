import { useEffect } from 'react'
import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom'
import { supabase } from './lib/supabase'
import { useAuthStore } from './store/store'

import AuthPage from './pages/AuthPage'
import Layout from './components/Layout'
import Dashboard from './pages/Dashboard'
import CatsPage from './pages/CatsPage'
import SchedulesPage from './pages/SchedulesPage'
import HistoryPage from './pages/HistoryPage'
import DevicePage from './pages/DevicePage'
import SettingsPage from './pages/SettingsPage'
import ChatPage from './pages/ChatPage'
import ToastContainer from './components/ToastContainer'

function PrivateRoute({ children }) {
  const { user, loading } = useAuthStore()
  if (loading) return <div className="auth-page"><div className="skeleton" style={{width:120,height:40,borderRadius:8}} /></div>
  return user ? children : <Navigate to="/auth" replace />
}

export default function App() {
  const { setSession, setLoading } = useAuthStore()

  useEffect(() => {
    supabase.auth.getSession().then(({ data: { session } }) => {
      setSession(session)
      setLoading(false)
    })

    const { data: { subscription } } = supabase.auth.onAuthStateChange((_event, session) => {
      setSession(session)
      setLoading(false)
    })

    return () => subscription.unsubscribe()
  }, [])

  return (
    <BrowserRouter>
      <ToastContainer />
      <Routes>
        <Route path="/auth" element={<AuthPage />} />
        <Route path="/" element={<PrivateRoute><Layout /></PrivateRoute>}>
          <Route index element={<Dashboard />} />
          <Route path="cats" element={<CatsPage />} />
          <Route path="schedules" element={<SchedulesPage />} />
          <Route path="history" element={<HistoryPage />} />
          <Route path="device" element={<DevicePage />} />
          <Route path="settings" element={<SettingsPage />} />
          <Route path="chat" element={<ChatPage />} />
        </Route>
        <Route path="*" element={<Navigate to="/" replace />} />
      </Routes>
    </BrowserRouter>
  )
}
