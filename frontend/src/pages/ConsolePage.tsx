import { useMemo } from "react";
import { useNavigate, useParams } from "react-router-dom";
import { ConsoleView, instanceLogSource, launcherLogSource } from "../components/ConsoleView";
import { useInstances } from "../hooks/stores";
import { useEvent } from "../hooks/useApi";

const LAUNCHER = "__launcher__";

export function ConsolePage() {
  const params = useParams();
  const navigate = useNavigate();
  const instances = useInstances();
  const selected = params.id ?? instances.data.find((i) => i.state !== "stopped")?.id ?? LAUNCHER;
  const source = useMemo(() => (selected === LAUNCHER ? launcherLogSource : instanceLogSource(selected)), [selected]);

  return (
    <div className="page">
      <div className="page-header">
        <h1>Console</h1>
        <div className="actions">
          <select
            className="select"
            value={selected}
            onChange={(e) => navigate(e.target.value === LAUNCHER ? "/console" : `/console/${encodeURIComponent(e.target.value)}`)}
            aria-label="Log source"
          >
            <option value={LAUNCHER}>Launcher log</option>
            {instances.data.map((i) => (
              <option key={i.id} value={i.id}>
                {i.name}
                {i.state === "running" ? " (running)" : i.state === "launching" ? " (launching)" : ""}
              </option>
            ))}
          </select>
        </div>
      </div>
      <ConsoleView key={selected} source={source} />
    </div>
  );
}

/**
 * The backend asks for the console when an instance has "show console" enabled
 * or crashed with "show console on error" (instance.consoleRequested).
 */
export function ConsoleRequestListener() {
  const navigate = useNavigate();
  useEvent("instance.consoleRequested", (e) => navigate(`/instances/${encodeURIComponent(e.instanceId)}/console`));
  return null;
}
