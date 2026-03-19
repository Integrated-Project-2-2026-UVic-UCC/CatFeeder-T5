import { useState } from 'react'
import { supabase } from '../lib/supabase'

export default function AuthPage() {
  const [mode, setMode] = useState('login') // 'login' | 'register' | 'reset'
  const [email, setEmail] = useState('')
  const [password, setPassword] = useState('')
  const [loading, setLoading] = useState(false)
  const [error, setError] = useState('')
  const [success, setSuccess] = useState('')

  const handleSubmit = async (e) => {
    e.preventDefault()
    setError(''); setSuccess('')
    setLoading(true)
    try {
      if (mode === 'login') {
        const { error } = await supabase.auth.signInWithPassword({ email, password })
        if (error) throw error
      } else if (mode === 'register') {
        const { error } = await supabase.auth.signUp({ email, password })
        if (error) throw error
        setSuccess('Check your email to confirm your account!')
      } else if (mode === 'reset') {
        const { error } = await supabase.auth.resetPasswordForEmail(email, {
          redirectTo: window.location.origin
        })
        if (error) throw error
        setSuccess('Password reset email sent!')
      }
    } catch (err) {
      setError(err.message)
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="auth-page">
      <div className="auth-card">
        <div className="auth-logo">
          <div className="auth-logo-icon">🐾</div>
          <div>
            <h1 style={{fontSize:'1.6rem'}}>CatFeeder T5</h1>
            <p style={{fontSize:'0.78rem',color:'var(--text-muted)',textAlign:'center'}}>Smart Feeding System</p>
          </div>
        </div>

        <h2 className="auth-title">
          {mode === 'login' ? 'Welcome back' : mode === 'register' ? 'Create account' : 'Reset password'}
        </h2>
        <p className="auth-subtitle">
          {mode === 'login' ? 'Sign in to manage your cats' : mode === 'register' ? 'Start monitoring your furry friends' : 'Enter your email to reset'}
        </p>

        <form className="auth-form" onSubmit={handleSubmit}>
          {error && <div className="auth-error">{error}</div>}
          {success && (
            <div style={{background:'rgba(74,222,128,0.1)',border:'1px solid rgba(74,222,128,0.3)',borderRadius:'var(--radius-sm)',padding:'10px 14px',fontSize:'0.84rem',color:'var(--green)'}}>
              {success}
            </div>
          )}

          <div className="form-group">
            <label className="form-label">Email</label>
            <input
              id="auth-email"
              type="email"
              className="form-input"
              placeholder="you@example.com"
              value={email}
              onChange={e => setEmail(e.target.value)}
              required
            />
          </div>

          {mode !== 'reset' && (
            <div className="form-group">
              <label className="form-label">Password</label>
              <input
                id="auth-password"
                type="password"
                className="form-input"
                placeholder="••••••••"
                value={password}
                onChange={e => setPassword(e.target.value)}
                required
                minLength={8}
              />
            </div>
          )}

          <button
            id="auth-submit"
            type="submit"
            className="btn btn-primary"
            style={{width:'100%',justifyContent:'center'}}
            disabled={loading}
          >
            {loading ? '◌ Loading...' : mode === 'login' ? 'Sign In' : mode === 'register' ? 'Create Account' : 'Send Reset Link'}
          </button>
        </form>

        <div className="auth-switch">
          {mode === 'login' && <>
            No account? <a onClick={() => setMode('register')}>Register</a>
            &nbsp;·&nbsp;
            <a onClick={() => setMode('reset')}>Forgot password?</a>
          </>}
          {mode === 'register' && <>
            Have an account? <a onClick={() => setMode('login')}>Sign in</a>
          </>}
          {mode === 'reset' && <>
            <a onClick={() => setMode('login')}>Back to sign in</a>
          </>}
        </div>
      </div>
    </div>
  )
}
