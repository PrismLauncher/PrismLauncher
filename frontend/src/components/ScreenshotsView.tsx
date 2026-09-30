import { useState } from "react";
import { materialmc } from "../api/client";
import { useQuery } from "../hooks/useApi";
import { ConfirmDialog, Dialog } from "./Dialog";
import { Empty, QueryView } from "./common";
import { formatDate } from "./format";
import { useToasts } from "./Toasts";
import type { Screenshot } from "../types/resources";

export function ScreenshotsView({ instanceId }: { instanceId: string }) {
  const query = useQuery(() => materialmc.screenshots.list(instanceId), [instanceId]);
  const { showError } = useToasts();
  const [selected, setSelected] = useState<Set<string>>(new Set());
  const [preview, setPreview] = useState<Screenshot>();
  const [confirm, setConfirm] = useState(false);

  return (
    <div className="stack">
      <div className="row">
        <span className="grow muted">{selected.size > 0 ? `${selected.size} selected` : "Click to select, double-click to view."}</span>
        {selected.size > 0 && (
          <button className="btn danger" onClick={() => setConfirm(true)}>
            Delete
          </button>
        )}
        <button
          className="btn"
          onClick={() => materialmc.system.openFolder({ target: "instance.screenshots", instanceId }).catch((e: unknown) => showError(e))}
        >
          Open folder
        </button>
      </div>
      <QueryView query={query}>
        {(shots) =>
          shots.length === 0 ? (
            <Empty>No screenshots yet (press F2 in game).</Empty>
          ) : (
            <div className="screenshot-grid">
              {shots.map((s) => (
                <figure
                  key={s.name}
                  className={selected.has(s.name) ? "selected" : ""}
                  onClick={() => {
                    const next = new Set(selected);
                    if (next.has(s.name)) next.delete(s.name);
                    else next.add(s.name);
                    setSelected(next);
                  }}
                  onDoubleClick={() => setPreview(s)}
                >
                  <img src={s.url} alt={s.name} loading="lazy" decoding="async" />
                  <figcaption className="ellipsis">
                    {s.name}
                    <div className="muted">{formatDate(s.modified)}</div>
                  </figcaption>
                </figure>
              ))}
            </div>
          )
        }
      </QueryView>
      {preview && (
        <Dialog title={preview.name} wide onClose={() => setPreview(undefined)}>
          <img src={preview.url} alt={preview.name} style={{ width: "100%", borderRadius: 8 }} />
        </Dialog>
      )}
      {confirm && (
        <ConfirmDialog
          title={`Delete ${selected.size} screenshot${selected.size > 1 ? "s" : ""}?`}
          danger
          confirmLabel="Delete"
          message="The image files will be deleted."
          onClose={() => setConfirm(false)}
          onConfirm={async () => {
            try {
              await materialmc.screenshots.remove({ instanceId, names: [...selected] });
              setSelected(new Set());
              query.reload();
            } catch (e) {
              showError(e, "Delete failed");
            }
          }}
        />
      )}
    </div>
  );
}
