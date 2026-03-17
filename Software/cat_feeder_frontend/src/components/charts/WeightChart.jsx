import {
    LineChart,
    Line,
    XAxis,
    YAxis,
    CartesianGrid,
    Tooltip,
    ResponsiveContainer,
} from 'recharts';

const CustomTooltip = ({ active, payload, label }) => {
    if (!active || !payload?.length) return null;
    return (
        <div className="glass-card px-3 py-2 text-xs shadow-xl">
            <p className="text-gray-400 mb-1">{label}</p>
            <p className="text-brand-400 font-bold">{payload[0]?.value?.toFixed(1)} g</p>
        </div>
    );
};

/**
 * @param {{ data: Array<{time: string, weight: number}>, loading: boolean }} props
 */
export default function WeightChart({ data = [], loading }) {
    return (
        <div className="glass-card p-5 ring-1 ring-brand-500/10">
            <div className="flex items-center justify-between mb-4">
                <div>
                    <p className="section-title">Bowl Weight Over Time</p>
                    <p className="text-xs text-gray-500 mt-0.5">Drops indicate eating events</p>
                </div>
                <span className="badge bg-brand-500/10 text-brand-400 ring-1 ring-brand-500/20">
                    ⚖️ Weight
                </span>
            </div>

            {loading ? (
                <div className="h-56 flex items-center justify-center text-gray-600 text-sm">
                    Waiting for sensor data…
                </div>
            ) : data.length === 0 ? (
                <div className="h-56 flex flex-col items-center justify-center gap-2 text-gray-600 text-sm">
                    <span className="text-3xl">📡</span>
                    No sensor logs yet
                </div>
            ) : (
                <ResponsiveContainer width="100%" height={220}>
                    <LineChart data={data} margin={{ top: 4, right: 4, left: -20, bottom: 0 }}>
                        <CartesianGrid strokeDasharray="3 3" stroke="rgba(255,255,255,0.04)" />
                        <XAxis
                            dataKey="time"
                            tick={{ fill: '#6b7280', fontSize: 10 }}
                            tickLine={false}
                            axisLine={false}
                            interval="preserveStartEnd"
                        />
                        <YAxis
                            tick={{ fill: '#6b7280', fontSize: 10 }}
                            tickLine={false}
                            axisLine={false}
                            unit="g"
                        />
                        <Tooltip content={<CustomTooltip />} />
                        <Line
                            type="monotone"
                            dataKey="weight"
                            stroke="#f5a623"
                            strokeWidth={2.5}
                            dot={false}
                            activeDot={{ r: 5, fill: '#f5a623', strokeWidth: 0 }}
                        />
                    </LineChart>
                </ResponsiveContainer>
            )}
        </div>
    );
}
