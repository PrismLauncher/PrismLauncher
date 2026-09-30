import { Empty, ErrorBanner, Spinner, TaskProgress } from "../components/common";
import { formatRelative } from "../components/format";
import { useToasts } from "../components/Toasts";
import { materialmc } from "../api/client";
import { refreshTasks, useTasks } from "../hooks/stores";

export function DownloadsPage() {
  const tasks = useTasks();
  const { showError } = useToasts();
  const running = tasks.data.filter((t) => t.state === "running");
  const finished = tasks.data.filter((t) => t.state !== "running");

  return (
    <div className="page">
      <div className="page-header">
        <h1>Downloads &amp; tasks</h1>
        <div className="actions">
          <button
            className="btn"
            disabled={finished.length === 0}
            onClick={() => materialmc.tasks.clearFinished().then(refreshTasks, (e: unknown) => showError(e))}
          >
            Clear finished
          </button>
        </div>
      </div>
      {tasks.error ? <ErrorBanner error={tasks.error} /> : !tasks.loaded ? <Spinner /> : null}

      <section className="stack">
        <h2>Active</h2>
        {running.length === 0 ? (
          <Empty>Nothing is downloading right now.</Empty>
        ) : (
          running.map((t) => (
            <div key={t.id} className="card row">
              <div className="grow">
                <TaskProgress task={t} />
                {t.details && <div className="small muted ellipsis">{t.details}</div>}
              </div>
              {t.canAbort && (
                <button className="btn small danger" onClick={() => materialmc.tasks.cancel(t.id).catch((e: unknown) => showError(e))}>
                  Cancel
                </button>
              )}
            </div>
          ))
        )}
      </section>

      {finished.length > 0 && (
        <section className="stack">
          <h2>Finished</h2>
          {finished.map((t) => (
            <div key={t.id} className="card row">
              <div className="grow">
                <TaskProgress task={t} />
              </div>
              <span className="small muted">{formatRelative(t.finishedAt)}</span>
            </div>
          ))}
        </section>
      )}
    </div>
  );
}
