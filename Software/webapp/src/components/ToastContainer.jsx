import { useAppStore } from '../store/store'

export default function ToastContainer() {
  const { toasts } = useAppStore()
  if (!toasts.length) return null

  return (
    <div className="toast-container">
      {toasts.map(t => (
        <div key={t.id} className={`toast ${t.type}`}>{t.message}</div>
      ))}
    </div>
  )
}
