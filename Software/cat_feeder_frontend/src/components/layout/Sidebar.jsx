import { NavLink } from 'react-router-dom';

const nav = [
    {
        to: '/',
        label: 'Dashboard',
        icon: (
            <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                <rect x="3" y="3" width="7" height="7" /><rect x="14" y="3" width="7" height="7" />
                <rect x="14" y="14" width="7" height="7" /><rect x="3" y="14" width="7" height="7" />
            </svg>
        ),
    },
    {
        to: '/cats',
        label: 'Cat Profiles',
        icon: (
            <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                <path d="M12 2a5 5 0 0 1 5 5v2a5 5 0 0 1-10 0V7a5 5 0 0 1 5-5z" />
                <path d="M2 21a10 10 0 0 1 20 0" />
            </svg>
        ),
    },
    {
        to: '/dietary-plans',
        label: 'Dietary Plans',
        icon: (
            <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                <path d="M18 8h1a4 4 0 0 1 0 8h-1" /><path d="M5 8h13v9a4 4 0 0 1-4 4H9a4 4 0 0 1-4-4z" />
                <line x1="9" y1="3" x2="9" y2="8" /><line x1="12" y1="3" x2="12" y2="8" />
                <line x1="15" y1="3" x2="15" y2="8" />
            </svg>
        ),
    },
    {
        to: '/settings',
        label: 'Settings',
        icon: (
            <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                <circle cx="12" cy="12" r="3" />
                <path d="M19.07 4.93a10 10 0 0 1 0 14.14M4.93 4.93a10 10 0 0 0 0 14.14" />
                <path d="M12 2v2M12 20v2M2 12h2M20 12h2" />
            </svg>
        ),
    },
];

export default function Sidebar() {
    return (
        <aside className="w-56 shrink-0 flex flex-col gap-1 px-2 py-4 border-r border-white/5 bg-surface-800/50">
            {/* Logo */}
            <div className="flex items-center gap-2.5 px-3 pb-5 pt-1">
                <div className="w-8 h-8 rounded-xl bg-brand-500 flex items-center justify-center text-sm shrink-0">
                    🐱
                </div>
                <div>
                    <p className="text-sm font-bold text-white leading-tight">CatFeeder</p>
                    <p className="text-[10px] text-gray-500 font-medium">Control Panel</p>
                </div>
            </div>

            {/* Nav links */}
            <nav className="flex flex-col gap-0.5">
                {nav.map(({ to, label, icon }) => (
                    <NavLink
                        key={to}
                        to={to}
                        end={to === '/'}
                        className={({ isActive }) =>
                            `nav-link ${isActive ? 'active' : ''}`
                        }
                    >
                        {icon}
                        <span>{label}</span>
                    </NavLink>
                ))}
            </nav>

            {/* Footer */}
            <div className="mt-auto px-3 pt-4 border-t border-white/5">
                <p className="text-[10px] text-gray-600">Backend: localhost:3000</p>
                <p className="text-[10px] text-gray-600">MQTT: localhost:1883</p>
            </div>
        </aside>
    );
}
