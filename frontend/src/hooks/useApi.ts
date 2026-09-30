import { useCallback, useEffect, useRef, useState } from "react";
import { on } from "../api/client";
import type { MaterialMCEventMap, MaterialMCEventName } from "../types/events";

/** Subscribes to a backend event for the lifetime of the component. */
export function useEvent<E extends MaterialMCEventName>(
  name: E,
  callback: (payload: MaterialMCEventMap[E]) => void,
): void {
  const ref = useRef(callback);
  ref.current = callback;
  useEffect(() => on(name, (payload) => ref.current(payload)), [name]);
}

export interface QueryState<T> {
  data: T | undefined;
  error: unknown;
  loading: boolean;
  reload: () => void;
}

/**
 * Runs an async backend call and re-runs it when `deps` change or `reload()` is called.
 * Out-of-order responses are ignored, so rapid dependency changes never show stale data.
 */
export function useQuery<T>(fetcher: () => Promise<T>, deps: readonly unknown[]): QueryState<T> {
  const [data, setData] = useState<T>();
  const [error, setError] = useState<unknown>(null);
  const [loading, setLoading] = useState(true);
  const [tick, setTick] = useState(0);
  const generation = useRef(0);

  useEffect(() => {
    const current = ++generation.current;
    setLoading(true);
    fetcher().then(
      (value) => {
        if (current !== generation.current) return;
        setData(value);
        setError(null);
        setLoading(false);
      },
      (e: unknown) => {
        if (current !== generation.current) return;
        setError(e);
        setLoading(false);
      },
    );
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [...deps, tick]);

  const reload = useCallback(() => setTick((t) => t + 1), []);
  return { data, error, loading, reload };
}

/** Coalesces bursts of calls into one trailing call after `ms` milliseconds. */
export function useDebouncedCallback(fn: () => void, ms: number): () => void {
  const timer = useRef<number | undefined>(undefined);
  const ref = useRef(fn);
  ref.current = fn;
  useEffect(() => () => window.clearTimeout(timer.current), []);
  return useCallback(() => {
    window.clearTimeout(timer.current);
    timer.current = window.setTimeout(() => ref.current(), ms);
  }, [ms]);
}

export function useDebouncedValue<T>(value: T, ms: number): T {
  const [debounced, setDebounced] = useState(value);
  useEffect(() => {
    const t = window.setTimeout(() => setDebounced(value), ms);
    return () => window.clearTimeout(t);
  }, [value, ms]);
  return debounced;
}
