import { useEffect, useMemo, useState } from "react";
import { NavLink, useNavigate, useParams } from "react-router-dom";
import { materialmc } from "../api/client";
import { ConsoleView, instanceLogSource } from "../components/ConsoleView";
import { Dialog } from "../components/Dialog";
import { ErrorBanner, Spinner } from "../components/common";
import { formatDate, formatDuration, formatRelative, LOADER_NAMES, plural } from "../components/format";
import { GameLogsView } from "../components/GameLogsView";
import { useInstanceActions } from "../components/InstanceActions";
import { InstanceStateChip, PlayButton } from "../components/InstanceCard";
import { InstanceSettingsView } from "../components/InstanceSettingsView";
import { MenuButton } from "../components/Menu";
import { ResourceList } from "../components/ResourceList";
import { ScreenshotsView } from "../components/ScreenshotsView";
import { useToasts } from "../components/Toasts";
import { WorldsView } from "../components/WorldsView";
import { useInstances } from "../hooks/stores";
import { useEvent, useQuery } from "../hooks/useApi";
import type { Instance } from "../types/instances";

const TABS = [
  { id: "overview", label: "Overview" },
  { id: "mods", label: "Mods" },
  { id: "resourcepacks", label: "Resource packs" },
  { id: "shaderpacks", label: "Shader packs" },
  { id: "texturepacks", label: "Texture packs" },
  { id: "worlds", label: "Worlds" },
  { id: "screenshots", label: "Screenshots" },
  { id: "console", label: "Console" },
  { id: "logs", label: "Logs" },
  { id: "settings", label: "Settings" },
] as const;

type TabId = (typeof TABS)[number]["id"];

export function InstanceDetailsPage() {
  const params = useParams();
  const id = params.id ?? "";
  const tab: TabId = TABS.some((t) => t.id === params.tab) ? (params.tab as TabId) : "overview";
  const instances = useInstances();
  const instance = instances.data.find((i) => i.id === id);
  const actions = useInstanceActions();
  const navigate = useNavigate();
  const { showError } = useToasts();

  if (!instance) {
    if (!instances.loaded) return <Spinner label="Loading instance…" />;
    if (instances.error) return <ErrorBanner error={instances.error} />;
    return (
      <div className="page">
        <ErrorBanner error={`Instance “${id}” does not exist (it may have been deleted).`} />
      </div>
    );
  }

  const base = `/instances/${encodeURIComponent(instance.id)}`;

  return (
    <div className="page">
      <div className="page-header">
        <img className="instance-icon" src={instance.iconUrl} alt="" />
        <div className="grow" style={{ minWidth: 0 }}>
          <h1 className="ellipsis">{instance.name}</h1>
          <div className="row small muted">
            Minecraft {instance.minecraftVersion ?? "?"} ·{" "}
            {instance.loader ? `${LOADER_NAMES[instance.loader.kind]} ${instance.loader.version}` : "Vanilla"}
            <InstanceStateChip instance={instance} />
          </div>
        </div>
        <div className="actions">
          <PlayButton instance={instance} />
          <MenuButton
            items={[
              { label: "Launch offline…", onSelect: () => actions.askOffline(instance), disabled: instance.state !== "stopped" },
              {
                label: "Open folder",
                separatorBefore: true,
                onSelect: () => materialmc.system.openFolder({ target: "instance.game", instanceId: instance.id }).catch((e: unknown) => showError(e)),
              },
              { label: "Rename…", onSelect: () => actions.rename(instance) },
              { label: "Duplicate…", onSelect: () => actions.duplicate(instance) },
              { label: "Delete…", danger: true, separatorBefore: true, disabled: instance.state !== "stopped", onSelect: () => actions.remove(instance) },
            ]}
          />
        </div>
      </div>

      <nav className="tabs">
        {TABS.map((t) => (
          <NavLink key={t.id} to={t.id === "overview" ? base : `${base}/${t.id}`} end className={tab === t.id ? "active" : ""}>
            {t.label}
          </NavLink>
        ))}
      </nav>

      {tab === "overview" && <Overview instance={instance} onNavigate={(t) => navigate(`${base}/${t}`)} />}
      {(tab === "mods" || tab === "resourcepacks" || tab === "shaderpacks" || tab === "texturepacks") && (
        <ResourceList instanceId={instance.id} kind={tab} />
      )}
      {tab === "worlds" && <WorldsView instanceId={instance.id} instanceName={instance.name} />}
      {tab === "screenshots" && <ScreenshotsView instanceId={instance.id} />}
      {tab === "console" && <InstanceConsole instanceId={instance.id} />}
      {tab === "logs" && <GameLogsView instanceId={instance.id} />}
      {tab === "settings" && <InstanceSettingsView instanceId={instance.id} />}
    </div>
  );
}

function InstanceConsole({ instanceId }: { instanceId: string }) {
  const source = useMemo(() => instanceLogSource(instanceId), [instanceId]);
  return <ConsoleView source={source} />;
}

function Overview({ instance, onNavigate }: { instance: Instance; onNavigate(tab: TabId): void }) {
  const details = useQuery(() => materialmc.instances.get(instance.id), [instance.id]);
  const { showError, notify } = useToasts();
  const [notes, setNotes] = useState("");
  const [iconPicker, setIconPicker] = useState(false);
  useEffect(() => setNotes(details.data?.notes ?? ""), [details.data?.notes]);
  useEvent("instances.changed", details.reload);

  return (
    <div className="grid-2" style={{ alignItems: "start" }}>
      <section className="card stack">
        <h2>Summary</h2>
        <dl className="stack" style={{ margin: 0, gap: 6 }}>
          <Info label="Last played" value={`${formatRelative(instance.lastLaunch)} (${formatDate(instance.lastLaunch)})`} />
          <Info label="Total play time" value={formatDuration(instance.totalPlayTime)} />
          <Info label="Last session" value={formatDuration(instance.lastPlayTime)} />
          <Info label="Group" value={instance.group ?? "Ungrouped"} />
          {instance.managedPack && (
            <Info label="Modpack" value={`${instance.managedPack.name} ${instance.managedPack.version} (${instance.managedPack.type})`} />
          )}
          {details.data && <Info label="Game folder" value={<code className="small">{details.data.gameRoot}</code>} />}
        </dl>
        <div className="row">
          <button className="btn small" onClick={() => onNavigate("mods")}>
            {plural(instance.modCount, "mod")}
          </button>
          <button className="btn small" onClick={() => onNavigate("worlds")}>
            Worlds
          </button>
          <button className="btn small" onClick={() => setIconPicker(true)}>
            Change icon
          </button>
        </div>
      </section>

      <section className="card stack">
        <h2>Components</h2>
        {details.error ? (
          <ErrorBanner error={details.error} onRetry={details.reload} />
        ) : !details.data ? (
          <Spinner />
        ) : (
          <table className="table">
            <tbody>
              {details.data.components.map((c) => (
                <tr key={c.uid} className={c.enabled ? "" : "disabled"}>
                  <td>{c.name}</td>
                  <td className="mono small">{c.version}</td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
        {/* TODO(webui): component editing (add/remove/change version) still lives in the Qt VersionPage. */}
      </section>

      <section className="card stack" style={{ gridColumn: "1 / -1" }}>
        <h2>Notes</h2>
        <textarea className="textarea" rows={5} value={notes} onChange={(e) => setNotes(e.target.value)} placeholder="Notes about this instance…" />
        <div className="row">
          <div className="grow" />
          <button
            className="btn"
            disabled={notes === (details.data?.notes ?? "")}
            onClick={() =>
              materialmc.instances.setNotes(instance.id, notes).then(() => notify("Notes saved", "success"), (e: unknown) => showError(e))
            }
          >
            Save notes
          </button>
        </div>
      </section>
      {iconPicker && <IconPicker instance={instance} onClose={() => setIconPicker(false)} />}
    </div>
  );
}

function Info({ label, value }: { label: string; value: React.ReactNode }) {
  return (
    <div className="row" style={{ alignItems: "baseline" }}>
      <dt className="muted small" style={{ width: 130 }}>
        {label}
      </dt>
      <dd style={{ margin: 0 }} className="grow">
        {value}
      </dd>
    </div>
  );
}

function IconPicker({ instance, onClose }: { instance: Instance; onClose(): void }) {
  const icons = useQuery(() => materialmc.system.icons(), []);
  const { showError } = useToasts();
  return (
    <Dialog title="Choose an icon" wide onClose={onClose}>
      {icons.error ? (
        <ErrorBanner error={icons.error} />
      ) : !icons.data ? (
        <Spinner />
      ) : (
        <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fill, minmax(72px, 1fr))", gap: 8 }}>
          {icons.data.map((icon) => (
            <button
              key={icon.key}
              className={`btn ghost${icon.key === instance.iconKey ? " primary" : ""}`}
              style={{ height: 72, borderRadius: 12 }}
              title={icon.name}
              onClick={() => materialmc.instances.setIcon(instance.id, icon.key).then(onClose, (e: unknown) => showError(e))}
            >
              <img src={icon.url} alt={icon.name} width={48} height={48} loading="lazy" style={{ imageRendering: "pixelated" }} />
            </button>
          ))}
        </div>
      )}
    </Dialog>
  );
}
