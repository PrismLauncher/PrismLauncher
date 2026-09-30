import { useEffect, useState } from "react";
import { Link } from "react-router-dom";
import { Empty, Spinner } from "../components/common";
import { WorldsView } from "../components/WorldsView";
import { useInstances } from "../hooks/stores";

export function WorldsPage() {
  const instances = useInstances();
  const [instanceId, setInstanceId] = useState("");

  useEffect(() => {
    if (!instanceId && instances.data.length > 0) {
      const recent = [...instances.data].sort((a, b) => (b.lastLaunch ?? 0) - (a.lastLaunch ?? 0))[0];
      if (recent) setInstanceId(recent.id);
    }
  }, [instanceId, instances.data]);

  const instance = instances.data.find((i) => i.id === instanceId);

  return (
    <div className="page">
      <div className="page-header">
        <h1>Worlds</h1>
        <div className="actions">
          <select className="select" value={instanceId} onChange={(e) => setInstanceId(e.target.value)} aria-label="Instance">
            {instances.data.map((i) => (
              <option key={i.id} value={i.id}>
                {i.name}
              </option>
            ))}
          </select>
        </div>
      </div>
      {!instances.loaded ? (
        <Spinner />
      ) : !instance ? (
        <Empty>
          No instances. <Link to="/instances/new">Create one</Link> first.
        </Empty>
      ) : (
        <WorldsView key={instance.id} instanceId={instance.id} instanceName={instance.name} />
      )}
    </div>
  );
}
