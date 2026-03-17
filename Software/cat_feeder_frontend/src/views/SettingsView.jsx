import { useState, useEffect, useCallback } from 'react';
import toast from 'react-hot-toast';
import TopBar from '../components/layout/TopBar';
import LoadingSpinner from '../components/ui/LoadingSpinner';
import { getHardwareSettings, updateHardwareSettings } from '../services/api';

const DEFAULTS = {
    calibration_factor: 450.0,
    motor_speed: 150,
    motor_steps: 800,
};

const CONFIG = [
    {
        key: 'calibration_factor',
        label: 'Calibration Factor',
        desc: 'Load cell scale factor. Adjust until weight readings are accurate.',
        min: 100, max: 1000, step: 1, unit: '', color: 'brand',
        icon: '⚖️',
    },
    {
        key: 'motor_speed',
        label: 'Motor Speed',
        desc: 'PWM speed of the dispensing motor (50–255). Higher = faster.',
        min: 50, max: 255, step: 1, unit: '', color: 'teal',
        icon: '⚡',
    },
    {
        key: 'motor_steps',
        label: 'Motor Steps',
        desc: 'Number of steps per dispense cycle. Controls portion size.',
        min: 100, max: 2000, step: 50, unit: '', color: 'violet',
        icon: '🔩',
    },
];

const trackColors = {
    brand: 'accent-brand-500',
    teal: 'accent-teal-500',
    violet: 'accent-violet-500',
};

export default function SettingsView() {
    const [values, setValues] = useState(DEFAULTS);
    const [loading, setLoading] = useState(true);
    const [saving, setSaving] = useState(false);

    const fetchSettings = useCallback(async () => {
        try {
            const data = await getHardwareSettings();
            const row = Array.isArray(data) ? data[0] : data?.settings ?? data;
            if (row) {
                setValues({
                    calibration_factor: row.calibration_factor ?? DEFAULTS.calibration_factor,
                    motor_speed: row.motor_speed ?? DEFAULTS.motor_speed,
                    motor_steps: row.motor_steps ?? DEFAULTS.motor_steps,
                });
            }
        } catch (_) {
            // backend not yet implemented — use defaults silently
        } finally {
            setLoading(false);
        }
    }, []);

    useEffect(() => { fetchSettings(); }, [fetchSettings]);

    const handleSlider = (key, val) =>
        setValues((v) => ({ ...v, [key]: parseFloat(val) }));

    const handleInput = (key, val) =>
        setValues((v) => ({ ...v, [key]: parseFloat(val) || 0 }));

    const handleSave = async () => {
        setSaving(true);
        const toastId = toast.loading('Saving settings…');
        try {
            await updateHardwareSettings(values);
            toast.success('Hardware settings saved ✅', { id: toastId });
        } catch (err) {
            toast.error(`Failed: ${err.message}`, { id: toastId });
        } finally {
            setSaving(false);
        }
    };

    const handleReset = () => {
        setValues(DEFAULTS);
        toast('Reset to defaults', { icon: '↩️' });
    };

    if (loading) {
        return (
            <div className="flex flex-col h-full">
                <TopBar title="Settings" subtitle="Hardware configuration" />
                <LoadingSpinner className="flex-1" />
            </div>
        );
    }

    return (
        <div className="flex flex-col h-full">
            <TopBar title="Settings" subtitle="Hardware configuration for the ESP32" />

            <div className="flex-1 overflow-y-auto p-6 space-y-5">
                {/* Warning */}
                <div className="bg-amber-500/5 border border-amber-500/20 rounded-2xl px-5 py-3 flex gap-3 items-start">
                    <span className="text-xl mt-0.5">⚠️</span>
                    <p className="text-xs text-amber-300/80">
                        Changes are written to the backend database and relayed to the ESP32 via MQTT.
                        Incorrect values may cause inaccurate feeding weights or motor stalls.
                    </p>
                </div>

                {/* Sliders */}
                {CONFIG.map(({ key, label, desc, min, max, step, unit, color, icon }) => (
                    <div key={key} className={`glass-card p-5 ring-1 ring-${color === 'brand' ? 'brand' : color}-500/10`}>
                        <div className="flex items-center gap-2 mb-1">
                            <span className="text-lg">{icon}</span>
                            <p className="font-semibold text-white text-sm">{label}</p>
                            <span className={`ml-auto font-mono font-bold text-lg ${color === 'brand' ? 'text-brand-400' :
                                    color === 'teal' ? 'text-teal-400' : 'text-violet-400'
                                }`}>
                                {values[key]}{unit}
                            </span>
                        </div>
                        <p className="text-xs text-gray-500 mb-3">{desc}</p>

                        <input
                            id={`slider-${key}`}
                            type="range"
                            min={min} max={max} step={step}
                            value={values[key]}
                            onChange={(e) => handleSlider(key, e.target.value)}
                            className={`w-full h-1.5 rounded-full appearance-none bg-surface-600 cursor-pointer ${trackColors[color]}`}
                        />
                        <div className="flex items-center justify-between mt-2 gap-3">
                            <span className="text-[10px] text-gray-600">{min}</span>
                            <input
                                id={`input-${key}`}
                                type="number"
                                value={values[key]}
                                min={min} max={max} step={step}
                                onChange={(e) => handleInput(key, e.target.value)}
                                className="input-field w-28 text-center text-sm py-1 font-mono"
                            />
                            <span className="text-[10px] text-gray-600">{max}</span>
                        </div>
                    </div>
                ))}

                {/* Actions */}
                <div className="flex gap-3 pt-1">
                    <button id="btn-save-settings" className="btn-primary flex-1" onClick={handleSave} disabled={saving}>
                        {saving ? 'Saving…' : '💾 Save Settings'}
                    </button>
                    <button id="btn-reset-settings" className="btn-secondary" onClick={handleReset}>
                        Reset Defaults
                    </button>
                </div>
            </div>
        </div>
    );
}
