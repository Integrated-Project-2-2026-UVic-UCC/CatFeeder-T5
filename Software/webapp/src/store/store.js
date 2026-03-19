import { create } from 'zustand'
import { supabase } from '../lib/supabase'

export const useAuthStore = create((set) => ({
  user: null,
  session: null,
  loading: true,
  setUser: (user) => set({ user }),
  setSession: (session) => set({ session, user: session?.user ?? null }),
  setLoading: (loading) => set({ loading }),
  signOut: async () => {
    await supabase.auth.signOut()
    set({ user: null, session: null })
  }
}))

export const useAppStore = create((set, get) => ({
  cats: [],
  devices: [],
  schedules: [],
  notifications: [],
  toasts: [],
  activeFeed: null,

  setCats: (cats) => set({ cats }),
  setDevices: (devices) => set({ devices }),
  setSchedules: (schedules) => set({ schedules }),
  setNotifications: (notifications) => set({ notifications }),
  setActiveFeed: (activeFeed) => set({ activeFeed }),

  addToast: (message, type = 'info') => {
    const id = Date.now()
    set((s) => ({ toasts: [...s.toasts, { id, message, type }] }))
    setTimeout(() => {
      set((s) => ({ toasts: s.toasts.filter(t => t.id !== id) }))
    }, 3500)
  },
  removeToast: (id) => set((s) => ({ toasts: s.toasts.filter(t => t.id !== id) })),

  fetchCats: async () => {
    const { data } = await supabase.from('cats').select('*').eq('archived', false).order('created_at')
    if (data) set({ cats: data })
  },
  fetchDevices: async () => {
    const { data } = await supabase.from('devices').select('*').order('created_at')
    if (data) set({ devices: data })
  },
  fetchSchedules: async () => {
    const { data } = await supabase.from('schedules').select('*, cats(name)').order('time_of_day')
    if (data) set({ schedules: data })
  },
  fetchNotifications: async () => {
    const { data } = await supabase.from('notifications').select('*').eq('read', false).order('created_at', { ascending: false }).limit(20)
    if (data) set({ notifications: data })
  },
}))
