import {
    LineChart,
    Line,
    XAxis,
    YAxis,
    CartesianGrid,
    Tooltip,
    Legend,
    ResponsiveContainer,
} from 'recharts';

const CustomTooltip = ({ active, payload, label }) => {
    if (!active || !payload?.length) return null;
    return (
        <div className="glass-card px-3 py-2 text-xs shadow-xl">
            <p className="text-gray-400 mb-1">{label}</p>
            {payload.map((p) => (
                <p key={p.dataKey} style={{ color: p.color }} className="font-bold">
                    {p.name}: {p.value?.toFixed(1)}{p.dataKey === 'temp' ? '°C' : '%'}
                </p>
            ))}
        </div>
    );
};

/**
 * @param {{ data: Array<{time: string, temp: number, humidity: number}>, loading: boolean }} props
 */
export default function EnvChart({ data = [], loading }) {
    return (
        <div className="glass-card p-5 ring-1 ring-teal-500/10">
            <div className="flex items-center justify-between mb-4">
                <div>
                    <p className="section-title">Environment Monitoring</p>
                    <p className="text-xs text-gray-500 mt-0.5">Temperature & Humidity for food quality</p>
                </div>
                <span className="badge bg-teal-500/10 text-teal-400 ring-1 ring-teal-500/20">
                    🌡️ Env
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
                        />
                        <Tooltip content={<CustomTooltip />} />
                        <Legend
                            formatter={(val) => <span style={{ color: '#9ca3af', fontSize: 11 }}>{val}</span>}
                            wrapperStyle={{ paddingTop: 8 }}
                        />
                        <Line
                            type="monotone"
                            dataKey="temp"
                            name="Temperature (°C)"
                            stroke="#2dd4bf"
                            strokeWidth={2}
                            dot={false}
                            activeDot={{ r: 4, strokeWidth: 0 }}
                        />
                        <Line
                            type="monotone"
                            dataKey="humidity"
                            name="Humidity (%)"
                            stroke="#818cf8"
                            strokeWidth={2}
                            dot={false}
                            activeDot={{ r: 4, strokeWidth: 0 }}
                        />
                    </LineChart>
                </ResponsiveContainer>
            )}
        </div>
    );
}
