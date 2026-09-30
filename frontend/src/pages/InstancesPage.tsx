import { useMemo, useState } from "react";
import { Link } from "react-router-dom";
import { materialmc } from "../api/client";
import { InstanceCard } from "../components/InstanceCard";
import { Empty, ErrorBanner, Spinner } from "../components/common";
import { useToasts } from "../components/Toasts";
import { useInstances } from "../hooks/stores";
import type { Instance } from "../types/instances";

type Sort = "name" | "lastLaunch" | "playtime";

const SORTERS: Record<Sort, (a: Instance, b: Instance) => number> = {
  name: (a, b) => a.name.localeCompare(b.name, undefined, { sensitivity: "base", numeric: true }),
  lastLaunch: (a, b) => (b.lastLaunch ?? 0) - (a.lastLaunch ?? 0),
  playtime: (a, b) => b.totalPlayTime - a.totalPlayTime,
};

export function InstancesPage() {
  const instances = useInstances();
  const { showError } = useToasts();
  const [query, setQuery] = useState("");
  const [sort, setSort] = useState<Sort>("name");

  const groups = useMemo(() => {
    const q = query.trim().toLowerCase();
    const filtered = instances.data.filter(
      (i) =>
        !q ||
        i.name.toLowerCase().includes(q) ||
        (i.minecraftVersion ?? "").includes(q) ||
        (i.loader?.kind ?? "vanilla").includes(q) ||
        (i.group ?? "").toLowerCase().includes(q),
    );
    const byGroup = new Map<string, Instance[]>();
    for (const i of filtered) {
      const key = i.group ?? "";
      const list = byGroup.get(key) ?? [];
      list.push(i);
      byGroup.set(key, list);
    }
    return [...byGroup.entries()]
      .sort(([a], [b]) => (a === "" ? 1 : b === "" ? -1 : a.localeCompare(b)))
      .map(([group, list]) => ({ group, list: list.sort(SORTERS[sort]) }));
  }, [instances.data, query, sort]);

  return (
    <div className="page">
      <div className="page-header">
        <h1>Instances</h1>
        <div className="actions">
          <input className="input" placeholder="Search instances…" value={query} onChange={(e) => setQuery(e.target.value)} />
          <select className="select" value={sort} onChange={(e) => setSort(e.target.value as Sort)} aria-label="Sort by">
            <option value="name">Name</option>
            <option value="lastLaunch">Last played</option>
            <option value="playtime">Play time</option>
          </select>
          <button
            className="btn"
            onClick={() => materialmc.system.openFolder({ target: "instances" }).catch((e: unknown) => showError(e))}
          >
            Open folder
          </button>
          <Link className="btn primary" to="/instances/new">
            + New instance
          </Link>
        </div>
      </div>

      {instances.error ? (
        <ErrorBanner error={instances.error} />
      ) : !instances.loaded ? (
        <Spinner label="Loading instances…" />
      ) : instances.data.length === 0 ? (
        <Empty>
          No instances yet. <Link to="/instances/new">Create your first instance</Link>.
        </Empty>
      ) : groups.length === 0 ? (
        <Empty>No instance matches “{query}”.</Empty>
      ) : (
        groups.map(({ group, list }) => (
          <section key={group} className="stack">
            <h2 className="muted">
              {group || "Ungrouped"} <span className="small">({list.length})</span>
            </h2>
            <div className="instance-grid">
              {list.map((i) => (
                <InstanceCard key={i.id} instance={i} />
              ))}
            </div>
          </section>
        ))
      )}
    </div>
  );
}
