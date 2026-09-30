import type { ReactNode } from "react";
import { describeError } from "./Toasts";
import type { TaskInfo } from "../types/tasks";

export function Spinner({ label }: { label?: string }) {
  return (
    <span className="row muted">
      <span className="spinner" aria-hidden /> {label}
    </span>
  );
}

export function Empty({ children }: { children: ReactNode }) {
  return <div className="empty">{children}</div>;
}

/** Inline error block with an optional retry button. */
export function ErrorBanner({ error, onRetry }: { error: unknown; onRetry?: () => void }) {
  const { message, code } = describeError(error);
  return (
    <div className="banner error row">
      <div className="grow">
        <strong>Something went wrong.</strong> {message}
        {code && <div className="small muted mono">{code}</div>}
      </div>
      {onRetry && (
        <button className="btn small" onClick={onRetry}>
          Retry
        </button>
      )}
    </div>
  );
}

/** Renders loading / error / content for a `useQuery` result. */
export function QueryView<T>({
  query,
  children,
  loadingLabel = "Loading…",
}: {
  query: { data: T | undefined; error: unknown; loading: boolean; reload(): void };
  children(data: T): ReactNode;
  loadingLabel?: string;
}) {
  if (query.error) return <ErrorBanner error={query.error} onRetry={query.reload} />;
  if (query.data === undefined) return <Spinner label={loadingLabel} />;
  return <>{children(query.data)}</>;
}

export function ProgressBar({ current, total }: { current: number; total: number }) {
  const determinate = total > 0;
  const pct = determinate ? Math.min(100, (current / total) * 100) : 0;
  return (
    <div className={`progress${determinate ? "" : " indeterminate"}`} role="progressbar" aria-valuenow={determinate ? pct : undefined}>
      <div style={determinate ? { width: `${pct}%` } : undefined} />
    </div>
  );
}

export function TaskProgress({ task }: { task: TaskInfo }) {
  return (
    <div className="stack" style={{ gap: 6 }}>
      <div className="row">
        <strong className="grow ellipsis">{task.title}</strong>
        <TaskStateChip task={task} />
      </div>
      {task.state === "running" && <ProgressBar current={task.current} total={task.total} />}
      <div className="small muted ellipsis">{task.state === "failed" ? task.error?.message || task.status : task.status}</div>
    </div>
  );
}

export function TaskStateChip({ task }: { task: TaskInfo }) {
  switch (task.state) {
    case "running":
      return <span className="chip info">{task.total > 0 ? `${Math.round((task.current / task.total) * 100)}%` : "Running"}</span>;
    case "succeeded":
      return <span className="chip ok">Done</span>;
    case "failed":
      return <span className="chip err">Failed</span>;
    case "aborted":
      return <span className="chip warn">Cancelled</span>;
  }
}
