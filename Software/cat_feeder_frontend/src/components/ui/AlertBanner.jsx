/**
 * Shows a dismissible warning banner.
 * @param {object} props
 * @param {'jam'|'stale'|null} props.type
 * @param {function} props.onDismiss
 */
export default function AlertBanner({ type, onDismiss }) {
    if (!type) return null;

    const config = {
        jam: {
            bg: 'bg-red-500/10 border-red-500/30',
            icon: '🚨',
            text: 'text-red-300',
            title: 'Anti-Jam Alert',
            message:
                'The motor has reported an error (jammed). Please check the feeder mechanism and clear any obstructions before the next dispensing cycle.',
        },
        stale: {
            bg: 'bg-amber-500/10 border-amber-500/30',
            icon: '⚠️',
            text: 'text-amber-300',
            title: 'Stale Food Warning',
            message:
                'Food has been sitting in the bowl for an extended period. Consider cleaning the bowl and refreshing the food supply.',
        },
    };

    const c = config[type];

    return (
        <div className={`flex items-start gap-3 p-4 rounded-2xl border ${c.bg} animate-slide-up`} role="alert">
            <span className="text-xl mt-0.5 shrink-0">{c.icon}</span>
            <div className="flex-1">
                <p className={`font-bold text-sm ${c.text}`}>{c.title}</p>
                <p className="text-xs text-gray-400 mt-0.5">{c.message}</p>
            </div>
            {onDismiss && (
                <button
                    onClick={onDismiss}
                    aria-label="Dismiss alert"
                    className="text-gray-500 hover:text-gray-300 transition-colors mt-0.5"
                >
                    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round">
                        <line x1="18" y1="6" x2="6" y2="18" /><line x1="6" y1="6" x2="18" y2="18" />
                    </svg>
                </button>
            )}
        </div>
    );
}
