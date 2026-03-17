export default function TopBar({ title, subtitle, statusDot }) {
    return (
        <header className="h-14 border-b border-white/5 flex items-center px-6 gap-4 shrink-0 bg-surface-800/30 backdrop-blur-sm">
            <div className="flex-1">
                <h1 className="text-base font-bold text-white leading-tight">{title}</h1>
                {subtitle && <p className="text-xs text-gray-500">{subtitle}</p>}
            </div>

            {/* Live status indicator */}
            <div className="flex items-center gap-2 text-xs text-gray-400">
                <span
                    className={`w-2 h-2 rounded-full ${statusDot === 'live'
                            ? 'bg-emerald-400 animate-pulse-slow'
                            : statusDot === 'error'
                                ? 'bg-red-400'
                                : 'bg-gray-600'
                        }`}
                />
                <span className="hidden sm:inline">
                    {statusDot === 'live' ? 'Live' : statusDot === 'error' ? 'Error' : 'Idle'}
                </span>
            </div>
        </header>
    );
}
