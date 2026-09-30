import { createContext, useCallback, useContext, useMemo, useState, type ReactNode } from "react";
import { useNavigate } from "react-router-dom";
import { materialmc } from "../api/client";
import { useEvent } from "../hooks/useApi";
import { instancesStore, useAccounts } from "../hooks/stores";
import { waitForTask } from "../hooks/useTaskCompletion";
import type { Instance, LaunchMode } from "../types/instances";
import type { ApiErrorCode } from "../types/common";
import { ConfirmDialog, Dialog } from "./Dialog";
import { useToasts } from "./Toasts";

interface InstanceActions {
  launch(instance: Pick<Instance, "id" | "name">, mode?: LaunchMode, offlineName?: string): Promise<void>;
  kill(instance: Pick<Instance, "id" | "name">): void;
  remove(instance: Pick<Instance, "id" | "name">): void;
  duplicate(instance: Pick<Instance, "id" | "name" | "group">): void;
  rename(instance: Pick<Instance, "id" | "name">): void;
  askOffline(instance: Pick<Instance, "id" | "name">): void;
}

const Ctx = createContext<InstanceActions | null>(null);

type DialogState =
  | { type: "kill"; instance: Pick<Instance, "id" | "name"> }
  | { type: "remove"; instance: Pick<Instance, "id" | "name"> }
  | { type: "duplicate"; instance: Pick<Instance, "id" | "name" | "group"> }
  | { type: "rename"; instance: Pick<Instance, "id" | "name"> }
  | { type: "offline"; instance: Pick<Instance, "id" | "name">; reason: string }
  | null;

/** Launch failures whose fix is "play offline instead". */
const OFFLINE_FALLBACK: ApiErrorCode[] = ["NO_ACCOUNT", "ACCOUNT_NEEDS_REAUTH", "ACCOUNT_NEEDS_PROFILE"];

export function InstanceActionsProvider({ children }: { children: ReactNode }) {
  const [dialog, setDialog] = useState<DialogState>(null);
  const { showError, notify } = useToasts();
  const navigate = useNavigate();
  const close = useCallback(() => setDialog(null), []);

  const launch = useCallback<InstanceActions["launch"]>(
    async (instance, mode = "normal", offlineName) => {
      try {
        await materialmc.instances.launch({ id: instance.id, mode, ...(offlineName ? { offlineName } : {}) });
      } catch (e) {
        showError(e, `Could not launch ${instance.name}`);
      }
    },
    [showError],
  );

  // The launch pipeline runs asynchronously in C++; failures arrive as events.
  useEvent("instance.launchFailed", (e) => {
    const name = instancesStore.getSnapshot().data.find((i) => i.id === e.instanceId)?.name ?? e.instanceId;
    if (OFFLINE_FALLBACK.includes(e.error.code)) {
      setDialog({ type: "offline", instance: { id: e.instanceId, name }, reason: e.error.message });
      return;
    }
    showError(e.error, `Launch of ${name} failed`);
  });
  useEvent("minecraft.crashed", (e) => showError(e.reason || "Minecraft crashed", "Game crashed"));

  const actions = useMemo<InstanceActions>(
    () => ({
      launch,
      kill: (instance) => setDialog({ type: "kill", instance }),
      remove: (instance) => setDialog({ type: "remove", instance }),
      duplicate: (instance) => setDialog({ type: "duplicate", instance }),
      rename: (instance) => setDialog({ type: "rename", instance }),
      askOffline: (instance) => setDialog({ type: "offline", instance, reason: "" }),
    }),
    [launch],
  );

  return (
    <Ctx.Provider value={actions}>
      {children}
      {dialog?.type === "kill" && (
        <ConfirmDialog
          title="Kill Minecraft?"
          danger
          confirmLabel="Kill"
          message={
            <>
              This force-stops <strong>{dialog.instance.name}</strong>. Unsaved progress may be lost and the world can get
              corrupted; only use it when the game is frozen.
            </>
          }
          onConfirm={() => materialmc.instances.kill(dialog.instance.id).then(() => undefined, (e) => showError(e))}
          onClose={close}
        />
      )}
      {dialog?.type === "remove" && (
        <ConfirmDialog
          title="Delete instance?"
          danger
          confirmLabel="Delete"
          message={
            <>
              <strong>{dialog.instance.name}</strong> and all its worlds, mods and settings will be moved to the trash (or
              deleted, if no trash is available).
            </>
          }
          onConfirm={async () => {
            try {
              await materialmc.instances.remove(dialog.instance.id);
              navigate("/instances");
            } catch (e) {
              showError(e, "Could not delete the instance");
            }
          }}
          onClose={close}
        />
      )}
      {dialog?.type === "duplicate" && (
        <DuplicateDialog
          instance={dialog.instance}
          onClose={close}
          onStarted={(taskId) => {
            close();
            notify(`Copying ${dialog.instance.name}…`);
            waitForTask(taskId).then(
              () => notify("Instance copied", "success"),
              (e: unknown) => showError(e, "Copy failed"),
            );
          }}
        />
      )}
      {dialog?.type === "rename" && <RenameDialog instance={dialog.instance} onClose={close} />}
      {dialog?.type === "offline" && (
        <OfflineDialog
          instanceName={dialog.instance.name}
          reason={dialog.reason}
          onClose={close}
          onLaunch={(name, mode) => {
            close();
            void launch(dialog.instance, mode, name);
          }}
        />
      )}
    </Ctx.Provider>
  );
}

export function useInstanceActions(): InstanceActions {
  const ctx = useContext(Ctx);
  if (!ctx) throw new Error("useInstanceActions must be used inside <InstanceActionsProvider>");
  return ctx;
}

function DuplicateDialog({
  instance,
  onClose,
  onStarted,
}: {
  instance: Pick<Instance, "id" | "name" | "group">;
  onClose(): void;
  onStarted(taskId: string): void;
}) {
  const [name, setName] = useState(`${instance.name} (copy)`);
  const [copySaves, setCopySaves] = useState(true);
  const [keepPlaytime, setKeepPlaytime] = useState(false);
  const [busy, setBusy] = useState(false);
  const { showError } = useToasts();

  return (
    <Dialog
      title={`Duplicate ${instance.name}`}
      onClose={onClose}
      footer={
        <>
          <button className="btn" onClick={onClose}>
            Cancel
          </button>
          <button
            className="btn primary"
            disabled={busy || !name.trim()}
            onClick={async () => {
              setBusy(true);
              try {
                const { taskId } = await materialmc.instances.copy({
                  id: instance.id,
                  name: name.trim(),
                  group: instance.group,
                  copySaves,
                  keepPlaytime,
                });
                onStarted(taskId);
              } catch (e) {
                showError(e, "Could not copy the instance");
                setBusy(false);
              }
            }}
          >
            Duplicate
          </button>
        </>
      }
    >
      <label className="field">
        <span>Name</span>
        <input className="input" value={name} onChange={(e) => setName(e.target.value)} autoFocus />
      </label>
      <label className="check">
        <input type="checkbox" checked={copySaves} onChange={(e) => setCopySaves(e.target.checked)} /> Copy worlds
      </label>
      <label className="check">
        <input type="checkbox" checked={keepPlaytime} onChange={(e) => setKeepPlaytime(e.target.checked)} /> Keep play
        time
      </label>
    </Dialog>
  );
}

function RenameDialog({ instance, onClose }: { instance: Pick<Instance, "id" | "name">; onClose(): void }) {
  const [name, setName] = useState(instance.name);
  const { showError } = useToasts();
  return (
    <Dialog
      title="Rename instance"
      onClose={onClose}
      footer={
        <>
          <button className="btn" onClick={onClose}>
            Cancel
          </button>
          <button
            className="btn primary"
            disabled={!name.trim()}
            onClick={() =>
              materialmc.instances.rename(instance.id, name.trim()).then(onClose, (e: unknown) => showError(e, "Rename failed"))
            }
          >
            Rename
          </button>
        </>
      }
    >
      <input className="input" value={name} onChange={(e) => setName(e.target.value)} autoFocus />
    </Dialog>
  );
}

function OfflineDialog({
  instanceName,
  reason,
  onClose,
  onLaunch,
}: {
  instanceName: string;
  reason: string;
  onClose(): void;
  onLaunch(name: string, mode: LaunchMode): void;
}) {
  const accounts = useAccounts();
  const navigate = useNavigate();
  const fallbackName = accounts.data.find((a) => a.isDefault)?.profileName ?? "Player";
  const [name, setName] = useState(fallbackName);
  const ownsGame = accounts.data.some((a) => a.ownsMinecraft);
  const valid = /^[A-Za-z0-9_]{1,16}$/.test(name);

  return (
    <Dialog
      title={`Play ${instanceName} offline?`}
      onClose={onClose}
      footer={
        <>
          <button
            className="btn"
            onClick={() => {
              onClose();
              navigate("/accounts");
            }}
          >
            Manage accounts
          </button>
          <button className="btn" onClick={onClose}>
            Cancel
          </button>
          <button className="btn primary" disabled={!valid} onClick={() => onLaunch(name, ownsGame ? "offline" : "demo")}>
            {ownsGame ? "Play offline" : "Play demo"}
          </button>
        </>
      }
    >
      {reason && <div className="banner warn">{reason}</div>}
      <p className="muted" style={{ margin: 0 }}>
        {ownsGame
          ? "Launch without contacting the Minecraft authentication servers."
          : "No account that owns Minecraft is signed in. You can play the demo, or add a Microsoft account."}
      </p>
      <label className="field">
        <span>Player name</span>
        <input className="input" value={name} maxLength={16} onChange={(e) => setName(e.target.value)} autoFocus />
        {!valid && <span className="hint">1–16 characters: letters, digits and underscores.</span>}
      </label>
    </Dialog>
  );
}
