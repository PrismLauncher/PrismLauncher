import { memo } from "react";
import { useNavigate } from "react-router-dom";
import { materialmc } from "../api/client";
import type { Instance } from "../types/instances";
import { formatDuration, formatRelative, LOADER_NAMES, plural } from "./format";
import { useInstanceActions } from "./InstanceActions";
import { MenuButton } from "./Menu";
import { useToasts } from "./Toasts";

export function InstanceStateChip({ instance }: { instance: Instance }) {
  if (instance.state === "running") return <span className="chip ok">Running</span>;
  if (instance.state === "launching") return <span className="chip info">Launching…</span>;
  if (instance.hasVersionBroken) return <span className="chip err">Broken version</span>;
  if (instance.hasCrashed) return <span className="chip warn">Crashed</span>;
  return null;
}

export function PlayButton({ instance, small }: { instance: Instance; small?: boolean }) {
  const actions = useInstanceActions();
  const cls = `btn${small ? " small" : ""}`;
  if (instance.state === "running") {
    return (
      <button className={`${cls} danger`} onClick={() => actions.kill(instance)}>
        ■ Stop
      </button>
    );
  }
  if (instance.state === "launching") {
    return (
      <button className={cls} onClick={() => actions.kill(instance)} title="Abort the launch">
        <span className="spinner" style={{ width: 14, height: 14 }} /> Cancel
      </button>
    );
  }
  return (
    <button className={`${cls} primary`} disabled={!instance.canLaunch} onClick={() => void actions.launch(instance)}>
      ▶ Play
    </button>
  );
}

export const InstanceCard = memo(function InstanceCard({ instance }: { instance: Instance }) {
  const navigate = useNavigate();
  const actions = useInstanceActions();
  const { showError } = useToasts();
  const open = () => navigate(`/instances/${encodeURIComponent(instance.id)}`);

  return (
    <article className="instance-card">
      <div className="head" onClick={open} role="link" tabIndex={0} onKeyDown={(e) => e.key === "Enter" && open()}>
        <img className="icon" src={instance.iconUrl} alt="" loading="lazy" />
        <div className="grow">
          <div className="title ellipsis" title={instance.name}>
            {instance.name}
          </div>
          <div className="muted small">Minecraft {instance.minecraftVersion ?? "?"}</div>
        </div>
      </div>
      <div className="meta">
        <span className="chip">{instance.loader ? LOADER_NAMES[instance.loader.kind] : "Vanilla"}</span>
        {instance.loader && <span className="chip">{plural(instance.modCount, "mod")}</span>}
        {instance.group && <span className="chip">{instance.group}</span>}
        <InstanceStateChip instance={instance} />
      </div>
      <div className="small muted">
        Played {formatRelative(instance.lastLaunch)} · {formatDuration(instance.totalPlayTime)}
      </div>
      <div className="foot">
        <PlayButton instance={instance} />
        <div className="grow" />
        <MenuButton
          items={[
            { label: "Open details", onSelect: open },
            { label: "Edit mods", onSelect: () => navigate(`/instances/${encodeURIComponent(instance.id)}/mods`) },
            { label: "Console", onSelect: () => navigate(`/console/${encodeURIComponent(instance.id)}`) },
            {
              label: "Launch offline…",
              onSelect: () => actions.askOffline(instance),
              disabled: instance.state !== "stopped",
            },
            {
              label: "Open folder",
              separatorBefore: true,
              onSelect: () =>
                materialmc.system.openFolder({ target: "instance.game", instanceId: instance.id }).catch((e: unknown) => showError(e)),
            },
            { label: "Rename…", onSelect: () => actions.rename(instance) },
            { label: "Duplicate…", onSelect: () => actions.duplicate(instance) },
            {
              label: "Delete…",
              danger: true,
              separatorBefore: true,
              disabled: instance.state !== "stopped",
              onSelect: () => actions.remove(instance),
            },
          ]}
        />
      </div>
    </article>
  );
});
