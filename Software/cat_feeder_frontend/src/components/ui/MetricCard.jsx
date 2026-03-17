/**
 * @param {object} props
 * @param {string}  props.label   - Card title (e.g. "Weight")
 * @param {string|number} props.value - Live value
 * @param {string}  props.unit    - Unit string (g / °C / %)
 * @param {string}  props.icon    - Emoji icon
 * @param {string}  [props.color] - Tailwind color variant: "amber" | "teal" | "violet"
 * @param {boolean} [props.loading]
 */
export default function MetricCard({ label, value, unit, icon, color = 'amber', loading }) {
    const accentMap = {
        amber: { ring: 'ring-brand-500/20', glow: 'bg-brand-500/10', text: 'text-brand-400' },
        teal: { ring: 'ring-teal-500/20', glow: 'bg-teal-500/10', text: 'text-teal-400' },
        violet: { ring: 'ring-violet-500/20', glow: 'bg-violet-500/10', text: 'text-violet-400' },
        rose: { ring: 'ring-rose-500/20', glow: 'bg-rose-500/10', text: 'text-rose-400' },
    };
    const a = accentMap[color] || accentMap.amber;

    return (
        <div className={`glass-card p-5 flex items-start gap-4 ring-1 ${a.ring} animate-fade-in`}>
            <div className={`w-10 h-10 rounded-xl ${a.glow} flex items-center justify-center text-xl shrink-0`}>
                {icon}
            </div>
            <div className="flex-1 min-w-0">
                <p className="label">{label}</p>
                {loading ? (
                    <div className="h-8 w-24 bg-surface-600 rounded-lg animate-pulse mt-1" />
                ) : (
                    <p className="metric-value">
                        {value ?? '—'}
                        <span className={`text-lg font-normal ml-1 ${a.text}`}>{unit}</span>
                    </p>
                )}
            </div>
        </div>
    );
}
