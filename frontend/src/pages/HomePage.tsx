import { Link } from "react-router-dom";
import { InstanceCard } from "../components/InstanceCard";
import { Empty, ErrorBanner, Spinner, TaskProgress } from "../components/common";
import { useAccounts, useInstances, useTasks } from "../hooks/stores";

export function HomePage() {
  const instances = useInstances();
  const tasks = useTasks();
  const accounts = useAccounts();

  const running = instances.data.filter((i) => i.state !== "stopped");
  const recent = instances.data
    .filter((i) => i.state === "stopped")
    .sort((a, b) => (b.lastLaunch ?? 0) - (a.lastLaunch ?? 0))
    .slice(0, 8);
  const activeTasks = tasks.data.filter((t) => t.state === "running");
  const noAccount = accounts.loaded && accounts.data.length === 0;

  return (
    <div className="page">
      <div className="page-header">
        <h1>Home</h1>
        <div className="actions">
          <Link className="btn primary" to="/instances/new">
            + New instance
          </Link>
        </div>
      </div>

      {noAccount && (
        <div className="banner warn row">
          <div className="grow">No Minecraft account is signed in. You need a Microsoft account that owns the game to play online.</div>
          <Link className="btn small" to="/accounts">
            Add account
          </Link>
        </div>
      )}

      {activeTasks.length > 0 && (
        <section className="card stack">
          <h2>In progress</h2>
          {activeTasks.slice(0, 4).map((t) => (
            <TaskProgress key={t.id} task={t} />
          ))}
          {activeTasks.length > 4 && (
            <Link to="/downloads" className="small">
              and {activeTasks.length - 4} more…
            </Link>
          )}
        </section>
      )}

      {running.length > 0 && (
        <section className="stack">
          <h2>Running</h2>
          <div className="instance-grid">
            {running.map((i) => (
              <InstanceCard key={i.id} instance={i} />
            ))}
          </div>
        </section>
      )}

      <section className="stack">
        <div className="row">
          <h2 className="grow">Recently played</h2>
          <Link to="/instances" className="small">
            All instances →
          </Link>
        </div>
        {instances.error ? (
          <ErrorBanner error={instances.error} />
        ) : !instances.loaded ? (
          <Spinner label="Loading instances…" />
        ) : recent.length === 0 && running.length === 0 ? (
          <Empty>
            You have no instances yet. <Link to="/instances/new">Create one</Link> to get started.
          </Empty>
        ) : (
          <div className="instance-grid">
            {recent.map((i) => (
              <InstanceCard key={i.id} instance={i} />
            ))}
          </div>
        )}
      </section>
    </div>
  );
}
