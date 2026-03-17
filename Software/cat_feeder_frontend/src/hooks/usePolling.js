import { useCallback, useEffect, useRef } from 'react';

/**
 * Polls `fn` every `intervalMs` milliseconds, immediately on mount.
 * Returns early if `fn` is called while a previous call is still in-flight.
 */
export function usePolling(fn, intervalMs = 5000, enabled = true) {
    const fnRef = useRef(fn);
    fnRef.current = fn;

    useEffect(() => {
        if (!enabled) return;
        let cancelled = false;

        const run = async () => {
            if (cancelled) return;
            try {
                await fnRef.current();
            } catch (_) {
                // errors handled inside fn
            }
        };

        run();
        const id = setInterval(run, intervalMs);
        return () => {
            cancelled = true;
            clearInterval(id);
        };
    }, [intervalMs, enabled]);
}
