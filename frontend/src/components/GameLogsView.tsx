import { useEffect, useMemo, useState } from "react";
import { materialmc } from "../api/client";
import { useQuery } from "../hooks/useApi";
import { Empty, ErrorBanner, QueryView, Spinner } from "./common";
import { formatBytes, formatDate } from "./format";
import { useToasts } from "./Toasts";
import { VirtualList } from "./VirtualList";

/** Browses the log files the game wrote to `logs/` and `crash-reports/`. */
export function GameLogsView({ instanceId }: { instanceId: string }) {
  const files = useQuery(() => materialmc.logs.list(instanceId), [instanceId]);
  const [name, setName] = useState<string>();
  const [search, setSearch] = useState("");
  const { showError, notify } = useToasts();

  useEffect(() => {
    if (!name && files.data && files.data.length > 0) setName(files.data[0]?.name);
  }, [files.data, name]);

  const content = useQuery(
    () => (name ? materialmc.logs.read({ instanceId, name }) : Promise.resolve(null)),
    [instanceId, name],
  );
  const lines = useMemo(() => {
    const all = content.data?.content.split("\n") ?? [];
    const q = search.trim().toLowerCase();
    return q ? all.filter((l) => l.toLowerCase().includes(q)) : all;
  }, [content.data, search]);

  return (
    <div className="stack">
      <QueryView query={files}>
        {(list) =>
          list.length === 0 ? (
            <Empty>This instance has no log files yet.</Empty>
          ) : (
            <>
              <div className="row">
                <select className="select grow" value={name ?? ""} onChange={(e) => setName(e.target.value)} aria-label="Log file">
                  {list.map((f) => (
                    <option key={f.name} value={f.name}>
                      {f.name} — {formatBytes(f.sizeBytes)} — {formatDate(f.modified)}
                    </option>
                  ))}
                </select>
                <input className="input" placeholder="Search…" value={search} onChange={(e) => setSearch(e.target.value)} />
                <button
                  className="btn"
                  disabled={!content.data}
                  onClick={() =>
                    content.data &&
                    materialmc.system.copyText(content.data.content).then(() => notify("Copied", "success"), (e: unknown) => showError(e))
                  }
                >
                  Copy
                </button>
                <button
                  className="btn"
                  onClick={() => materialmc.system.openFolder({ target: "instance.logs", instanceId }).catch((e: unknown) => showError(e))}
                >
                  Open folder
                </button>
              </div>
              {content.data?.truncated && <div className="banner warn small">Large file: showing only the last part.</div>}
              {content.error ? (
                <ErrorBanner error={content.error} onRetry={content.reload} />
              ) : content.loading ? (
                <Spinner />
              ) : (
                <VirtualList
                  className="console"
                  style={{ height: "calc(100vh - 360px)", minHeight: 240 }}
                  items={lines}
                  rowHeight={20}
                  getKey={(_, i) => i}
                  renderRow={(line) => <div className="line">{line}</div>}
                />
              )}
            </>
          )
        }
      </QueryView>
    </div>
  );
}
