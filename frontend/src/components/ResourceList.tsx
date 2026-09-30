import { useMemo, useState } from "react";
import { Link } from "react-router-dom";
import { materialmc } from "../api/client";
import { useEvent, useQuery } from "../hooks/useApi";
import type { Resource, ResourceKind } from "../types/resources";
import { ConfirmDialog } from "./Dialog";
import { Empty, ErrorBanner, Spinner } from "./common";
import { formatBytes, formatDate } from "./format";
import { useToasts } from "./Toasts";

export const RESOURCE_LABELS: Record<ResourceKind, { title: string; singular: string; folder: `instance.${ResourceKind}` }> = {
  mods: { title: "Mods", singular: "mod", folder: "instance.mods" },
  resourcepacks: { title: "Resource packs", singular: "resource pack", folder: "instance.resourcepacks" },
  shaderpacks: { title: "Shader packs", singular: "shader pack", folder: "instance.shaderpacks" },
  texturepacks: { title: "Texture packs", singular: "texture pack", folder: "instance.texturepacks" },
};

/** Table of the files in one resource folder of an instance, with enable/disable/remove/import. */
export function ResourceList({ instanceId, kind }: { instanceId: string; kind: ResourceKind }) {
  const { showError, notify } = useToasts();
  const query = useQuery(() => materialmc.resources.list({ instanceId, kind }), [instanceId, kind]);
  const [filter, setFilter] = useState("");
  const [selection, setSelection] = useState<Set<string>>(new Set());
  const [confirmRemove, setConfirmRemove] = useState(false);
  const labels = RESOURCE_LABELS[kind];

  useEvent("resources.changed", (e) => {
    if (e.instanceId === instanceId && e.kind === kind) query.reload();
  });

  const visible = useMemo(() => {
    const q = filter.trim().toLowerCase();
    const list = query.data ?? [];
    return q ? list.filter((r) => r.name.toLowerCase().includes(q) || r.fileName.toLowerCase().includes(q)) : list;
  }, [query.data, filter]);

  const selectedIds = [...selection].filter((id) => visible.some((r) => r.id === id));

  const setEnabled = async (ids: string[], enabled: boolean) => {
    try {
      await materialmc.resources.setEnabled({ instanceId, kind, ids, enabled });
      setSelection(new Set());
      query.reload();
    } catch (e) {
      showError(e);
    }
  };

  const toggle = (r: Resource) => {
    const next = new Set(selection);
    if (next.has(r.id)) next.delete(r.id);
    else next.add(r.id);
    setSelection(next);
  };

  return (
    <div className="stack">
      <div className="row">
        <input className="input grow" placeholder={`Filter ${labels.title.toLowerCase()}…`} value={filter} onChange={(e) => setFilter(e.target.value)} />
        <Link className="btn primary" to={`/mods?instance=${encodeURIComponent(instanceId)}&kind=${kind}`}>
          Download…
        </Link>
        <button
          className="btn"
          onClick={async () => {
            try {
              const { imported } = await materialmc.resources.importFiles({ instanceId, kind });
              if (imported > 0) notify(`Added ${imported} file${imported > 1 ? "s" : ""}`, "success");
              query.reload();
            } catch (e) {
              showError(e, "Import failed");
            }
          }}
        >
          Add files…
        </button>
        <button
          className="btn"
          onClick={() => materialmc.system.openFolder({ target: labels.folder, instanceId }).catch((e: unknown) => showError(e))}
        >
          Open folder
        </button>
      </div>

      {selectedIds.length > 0 && (
        <div className="row card" style={{ padding: 8 }}>
          <span className="grow">{selectedIds.length} selected</span>
          <button className="btn small" onClick={() => void setEnabled(selectedIds, true)}>
            Enable
          </button>
          <button className="btn small" onClick={() => void setEnabled(selectedIds, false)}>
            Disable
          </button>
          <button className="btn small danger" onClick={() => setConfirmRemove(true)}>
            Remove
          </button>
        </div>
      )}

      {query.error ? (
        <ErrorBanner error={query.error} onRetry={query.reload} />
      ) : !query.data ? (
        <Spinner />
      ) : query.data.length === 0 ? (
        <Empty>No {labels.title.toLowerCase()} installed.</Empty>
      ) : (
        <div className="card flush" style={{ overflow: "auto", maxHeight: "calc(100vh - 320px)" }}>
          <table className="table">
            <thead>
              <tr>
                <th style={{ width: 32 }}>
                  <input
                    type="checkbox"
                    aria-label="Select all"
                    checked={visible.length > 0 && selectedIds.length === visible.length}
                    onChange={(e) => setSelection(e.target.checked ? new Set(visible.map((r) => r.id)) : new Set())}
                  />
                </th>
                <th style={{ width: 64 }}>Enabled</th>
                <th>Name</th>
                <th>Version</th>
                <th>Provider</th>
                <th>Size</th>
                <th>Modified</th>
              </tr>
            </thead>
            <tbody>
              {visible.map((r) => (
                <tr key={r.id} className={`${selection.has(r.id) ? "selected" : ""} ${r.enabled ? "" : "disabled"}`}>
                  <td>
                    <input type="checkbox" aria-label={`Select ${r.name}`} checked={selection.has(r.id)} onChange={() => toggle(r)} />
                  </td>
                  <td>
                    <input
                      type="checkbox"
                      aria-label={r.enabled ? "Disable" : "Enable"}
                      checked={r.enabled}
                      onChange={(e) => void setEnabled([r.id], e.target.checked)}
                    />
                  </td>
                  <td>
                    <div className="ellipsis" style={{ maxWidth: 360 }} title={r.fileName}>
                      {r.name}
                    </div>
                    {r.issues.length > 0 && <div className="small" style={{ color: "var(--warning)" }}>{r.issues.join("; ")}</div>}
                  </td>
                  <td className="small">{r.version || "—"}</td>
                  <td className="small">{r.provider ?? "—"}</td>
                  <td className="small">{formatBytes(r.sizeBytes)}</td>
                  <td className="small muted">{formatDate(r.modified)}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}

      {confirmRemove && (
        <ConfirmDialog
          title={`Remove ${selectedIds.length} ${labels.singular}${selectedIds.length > 1 ? "s" : ""}?`}
          danger
          confirmLabel="Remove"
          message="The files will be deleted from the instance folder."
          onClose={() => setConfirmRemove(false)}
          onConfirm={async () => {
            try {
              await materialmc.resources.remove({ instanceId, kind, ids: selectedIds });
              setSelection(new Set());
              query.reload();
            } catch (e) {
              showError(e, "Remove failed");
            }
          }}
        />
      )}
    </div>
  );
}
