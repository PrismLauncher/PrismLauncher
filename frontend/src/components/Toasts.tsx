import { createContext, useCallback, useContext, useMemo, useState, type ReactNode } from "react";
import { isMaterialMCError } from "../types/common";

type ToastKind = "error" | "success" | "info";

interface Toast {
  id: number;
  kind: ToastKind;
  message: string;
  code?: string;
}

interface ToastApi {
  showError(error: unknown, context?: string): void;
  notify(message: string, kind?: ToastKind): void;
}

const ToastContext = createContext<ToastApi | null>(null);

export function describeError(error: unknown): { message: string; code?: string } {
  if (isMaterialMCError(error)) return { message: error.message, code: error.code };
  if (error instanceof Error) return { message: error.message };
  return { message: String(error) };
}

let nextId = 1;

export function ToastProvider({ children }: { children: ReactNode }) {
  const [toasts, setToasts] = useState<Toast[]>([]);

  const dismiss = useCallback((id: number) => setToasts((list) => list.filter((t) => t.id !== id)), []);

  const push = useCallback(
    (toast: Omit<Toast, "id">) => {
      const id = nextId++;
      setToasts((list) => [...list.slice(-4), { ...toast, id }]);
      window.setTimeout(() => dismiss(id), toast.kind === "error" ? 9000 : 4000);
    },
    [dismiss],
  );

  const api = useMemo<ToastApi>(
    () => ({
      showError(error, context) {
        const { message, code } = describeError(error);
        if (code === "CANCELLED") return;
        push({ kind: "error", message: context ? `${context}: ${message}` : message, code });
      },
      notify(message, kind = "info") {
        push({ kind, message });
      },
    }),
    [push],
  );

  return (
    <ToastContext.Provider value={api}>
      {children}
      <div className="toasts" role="status" aria-live="polite">
        {toasts.map((t) => (
          <div key={t.id} className={`toast ${t.kind}`}>
            <div className="grow">
              <div>{t.message}</div>
              {t.code && <div className="code">{t.code}</div>}
            </div>
            <button className="btn ghost small icon-only" aria-label="Dismiss" onClick={() => dismiss(t.id)}>
              ✕
            </button>
          </div>
        ))}
      </div>
    </ToastContext.Provider>
  );
}

export function useToasts(): ToastApi {
  const api = useContext(ToastContext);
  if (!api) throw new Error("useToasts must be used inside <ToastProvider>");
  return api;
}
