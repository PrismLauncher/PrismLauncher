import { useState } from "react";
import { materialmc } from "../api/client";
import { useEvent, useQuery } from "../hooks/useApi";
import { waitForTask } from "../hooks/useTaskCompletion";
import type { World } from "../types/resources";
import { ConfirmDialog, Dialog } from "./Dialog";
import { Empty, QueryView } from "./common";
import { formatBytes, formatRelative } from "./format";
import { MenuButton } from "./Menu";
import { useToasts } from "./Toasts";

export function WorldsView({ instanceId, instanceName }: { instanceId: string; instanceName: string }) {
  const query = useQuery(() => materialmc.worlds.list(instanceId), [instanceId]);
  const { showError, notify } = useToasts();
  const [removing, setRemoving] = useState<World>();
  const [renaming, setRenaming] = useState<World>();
  const [newName, setNewName] = useState("");

  useEvent("worlds.changed", (e) => e.instanceId === instanceId && query.reload());

  return (
    <div className="stack">
      <div className="row">
        <div className="grow" />
        <button
          className="btn"
          onClick={() => materialmc.system.openFolder({ target: "instance.saves", instanceId }).catch((e: unknown) => showError(e))}
        >
          Open saves folder
        </button>
      </div>
      <QueryView query={query}>
        {(worlds) =>
          worlds.length === 0 ? (
            <Empty>No worlds yet. Play the game to create one.</Empty>
          ) : (
            <div className="card flush">
              {worlds.map((w) => (
                <div key={w.id} className="list-row" style={{ padding: 12 }}>
                  {w.iconUrl ? (
                    <img className="instance-icon" src={w.iconUrl} alt="" loading="lazy" />
                  ) : (
                    <div className="instance-icon" />
                  )}
                  <div className="grow" style={{ minWidth: 0 }}>
                    <div className="ellipsis">
                      <strong>{w.name}</strong> <span className="muted small">({w.id})</span>
                    </div>
                    <div className="small muted">
                      {w.gameType} · played {formatRelative(w.lastPlayed)} · {formatBytes(w.sizeBytes)}
                      {!w.isValid && <span className="chip err" style={{ marginLeft: 8 }}>Invalid</span>}
                    </div>
                  </div>
                  <button className="btn small primary" onClick={() => void launchWorld(w)}>
                    ▶ Join
                  </button>
                  <MenuButton
                    items={[
                      { label: "Copy seed", onSelect: () => void materialmc.system.copyText(w.seed) },
                      {
                        label: "Rename…",
                        onSelect: () => {
                          setNewName(w.name);
                          setRenaming(w);
                        },
                      },
                      { label: "Delete…", danger: true, separatorBefore: true, onSelect: () => setRemoving(w) },
                    ]}
                  />
                </div>
              ))}
            </div>
          )
        }
      </QueryView>

      {removing && (
        <ConfirmDialog
          title={`Delete “${removing.name}”?`}
          danger
          confirmLabel="Delete"
          message="The world folder will be deleted permanently. This cannot be undone."
          onClose={() => setRemoving(undefined)}
          onConfirm={async () => {
            try {
              const { taskId } = await materialmc.worlds.remove({ instanceId, worldId: removing.id });
              await waitForTask(taskId);
              notify("World deleted", "success");
              query.reload();
            } catch (e) {
              showError(e, "Delete failed");
            }
          }}
        />
      )}
      {renaming && (
        <Dialog
          title="Rename world"
          onClose={() => setRenaming(undefined)}
          footer={
            <>
              <button className="btn" onClick={() => setRenaming(undefined)}>
                Cancel
              </button>
              <button
                className="btn primary"
                disabled={!newName.trim()}
                onClick={async () => {
                  try {
                    await materialmc.worlds.rename({ instanceId, worldId: renaming.id, name: newName.trim() });
                    setRenaming(undefined);
                    query.reload();
                  } catch (e) {
                    showError(e, "Rename failed");
                  }
                }}
              >
                Rename
              </button>
            </>
          }
        >
          <input className="input" value={newName} onChange={(e) => setNewName(e.target.value)} autoFocus />
        </Dialog>
      )}
    </div>
  );

  async function launchWorld(world: World) {
    try {
      await materialmc.instances.launch({ id: instanceId, world: world.id });
    } catch (e) {
      showError(e, `Could not launch ${instanceName}`);
    }
  }
}
