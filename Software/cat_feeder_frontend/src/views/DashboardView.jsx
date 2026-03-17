import { useState, useCallback } from 'react';
import TopBar from '../components/layout/TopBar';
import MetricCard from '../components/ui/MetricCard';
import AlertBanner from '../components/ui/AlertBanner';
import FeedButton from '../components/ui/FeedButton';
import WeightChart from '../components/charts/WeightChart';
import EnvChart from '../components/charts/EnvChart';
import { getSensorLogs } from '../services/api';
import { usePolling } from '../hooks/usePolling';

function formatTime(iso) {
    if (!iso) return '';
    const d = new Date(iso);
    return d.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
}

export default function DashboardView() {
    const [logs, setLogs] = useState([]);
    const [loading, setLoading] = useState(true);
    const [statusDot, setStatusDot] = useState('idle');
    const [alert, setAlert] = useState(null); // 'jam' | 'stale' | null

    const fetchLogs = useCallback(async () => {
        try {
            const data = await getSensorLogs(80);
            const rows = Array.isArray(data) ? data : data?.logs ?? [];
            setLogs(rows);
            setStatusDot('live');

            // Detect jam from latest entry
            const latest = rows[rows.length - 1];
            if (latest?.motor_status === 'error') {
                setAlert('jam');
            }
        } catch (_) {
            setStatusDot('error');
        } finally {
            setLoading(false);
        }
    }, []);

    usePolling(fetchLogs, 5000);

    // Latest readings
    const latestWithWeight = [...logs].reverse().find((l) => l.current_weight_grams != null);
    const latestWithEnv = [...logs].reverse().find((l) => l.temperature != null);

    // Chart data
    const weightData = logs
        .filter((l) => l.current_weight_grams != null)
        .map((l) => ({ time: formatTime(l.created_at ?? l.timestamp), weight: l.current_weight_grams }));

    const envData = logs
        .filter((l) => l.temperature != null)
        .map((l) => ({
            time: formatTime(l.created_at ?? l.timestamp),
            temp: l.temperature,
            humidity: l.humidity,
        }));

    return (
        <div className="flex flex-col h-full">
            <TopBar
                title="Dashboard"
                subtitle="Real-time telemetry from your cat feeder"
                statusDot={statusDot}
            />

            <div className="flex-1 overflow-y-auto p-6 space-y-6">
                {/* Alert banner */}
                {alert && (
                    <AlertBanner type={alert} onDismiss={() => setAlert(null)} />
                )}

                {/* Metric cards */}
                <div className="grid grid-cols-1 sm:grid-cols-3 gap-4">
                    <MetricCard
                        label="Bowl Weight"
                        value={latestWithWeight?.current_weight_grams?.toFixed(1)}
                        unit="g"
                        icon="⚖️"
                        color="amber"
                        loading={loading}
                    />
                    <MetricCard
                        label="Temperature"
                        value={latestWithEnv?.temperature?.toFixed(1)}
                        unit="°C"
                        icon="🌡️"
                        color="teal"
                        loading={loading}
                    />
                    <MetricCard
                        label="Humidity"
                        value={latestWithEnv?.humidity?.toFixed(1)}
                        unit="%"
                        icon="💧"
                        color="violet"
                        loading={loading}
                    />
                </div>

                {/* Manual feed + quick stats */}
                <div className="grid grid-cols-1 sm:grid-cols-3 gap-4">
                    <div className="glass-card p-5 ring-1 ring-brand-500/20 sm:col-span-1 flex flex-col gap-4">
                        <p className="section-title">Manual Control</p>
                        <p className="text-xs text-gray-500">
                            Immediately dispense food by triggering the motor via an MQTT command.
                        </p>
                        <FeedButton />
                    </div>

                    <div className="glass-card p-5 ring-1 ring-white/5 sm:col-span-2 flex flex-col gap-3">
                        <p className="section-title">Quick Stats</p>
                        <div className="grid grid-cols-2 gap-3 mt-1">
                            <div className="bg-surface-700 rounded-xl p-3">
                                <p className="label">Total Events</p>
                                <p className="text-2xl font-bold text-white font-mono">{logs.length}</p>
                            </div>
                            <div className="bg-surface-700 rounded-xl p-3">
                                <p className="label">Last Seen</p>
                                <p className="text-sm font-semibold text-white font-mono">
                                    {logs.length ? formatTime(logs[logs.length - 1]?.created_at ?? logs[logs.length - 1]?.timestamp) : '—'}
                                </p>
                            </div>
                            <div className="bg-surface-700 rounded-xl p-3">
                                <p className="label">Motor Status</p>
                                <p className={`text-sm font-bold font-mono ${latestWithWeight?.motor_status === 'error' ? 'text-red-400' : 'text-emerald-400'}`}>
                                    {latestWithWeight?.motor_status ?? 'OK'}
                                </p>
                            </div>
                            <div className="bg-surface-700 rounded-xl p-3">
                                <p className="label">Polling</p>
                                <p className="text-sm font-semibold text-teal-400 font-mono">5 s</p>
                            </div>
                        </div>
                    </div>
                </div>

                {/* Charts */}
                <WeightChart data={weightData} loading={loading} />
                <EnvChart data={envData} loading={loading} />
            </div>
        </div>
    );
}
