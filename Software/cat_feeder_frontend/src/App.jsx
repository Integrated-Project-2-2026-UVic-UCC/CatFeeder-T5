import { BrowserRouter, Routes, Route } from 'react-router-dom';
import { Toaster } from 'react-hot-toast';
import Sidebar from './components/layout/Sidebar';
import DashboardView from './views/DashboardView';
import CatProfilesView from './views/CatProfilesView';
import DietaryPlansView from './views/DietaryPlansView';
import SettingsView from './views/SettingsView';

export default function App() {
  return (
    <BrowserRouter>
      <div className="flex h-full">
        <Sidebar />
        <main className="flex-1 flex flex-col min-h-0 overflow-hidden">
          <Routes>
            <Route path="/" index element={<DashboardView />} />
            <Route path="/cats" element={<CatProfilesView />} />
            <Route path="/dietary-plans" element={<DietaryPlansView />} />
            <Route path="/settings" element={<SettingsView />} />
            <Route path="*" element={
              <div className="flex flex-col items-center justify-center h-full gap-3 text-gray-600">
                <span className="text-6xl">🐾</span>
                <p className="text-lg font-semibold">Page not found</p>
              </div>
            } />
          </Routes>
        </main>
      </div>

      <Toaster
        position="bottom-right"
        toastOptions={{
          style: {
            background: '#1a1d2e',
            color: '#e5e7eb',
            border: '1px solid rgba(255,255,255,0.06)',
            fontSize: '13px',
            borderRadius: '12px',
          },
          success: { iconTheme: { primary: '#f5a623', secondary: '#0a0b10' } },
          error: { iconTheme: { primary: '#f87171', secondary: '#0a0b10' } },
        }}
      />
    </BrowserRouter>
  );
}
