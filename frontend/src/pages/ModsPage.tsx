import { useEffect, useState } from "react";
import { Link, useSearchParams } from "react-router-dom";
import { materialmc } from "../api/client";
import { Dialog } from "../components/Dialog";
import { Empty, ErrorBanner, Spinner } from "../components/common";
import { RESOURCE_LABELS } from "../components/ResourceList";
import { useToasts } from "../components/Toasts";
import { useInstances } from "../hooks/stores";
import { useDebouncedValue, useQuery } from "../hooks/useApi";
import { waitForTask } from "../hooks/useTaskCompletion";
import type { Instance } from "../types/instances";
import type { ModProvider, ModSearchResult, RemoteProject } from "../types/mods";
import type { ResourceKind } from "../types/resources";

const KINDS: ResourceKind[] = ["mods", "resourcepacks", "shaderpacks", "texturepacks"];

export function ModsPage() {
  const instances = useInstances();
  const [params, setParams] = useSearchParams();
  const instanceId = params.get("instance") ?? "";
  const kind = (KINDS.find((k) => k === params.get("kind")) ?? "mods") as ResourceKind;
  const instance = instances.data.find((i) => i.id === instanceId);

  const update = (key: string, value: string) => {
    const next = new URLSearchParams(params);
    next.set(key, value);
    setParams(next, { replace: true });
  };

  // Default to the most recently played instance.
  useEffect(() => {
    if (!instanceId && instances.data.length > 0) {
      const recent = [...instances.data].sort((a, b) => (b.lastLaunch ?? 0) - (a.lastLaunch ?? 0))[0];
      if (recent) update("instance", recent.id);
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [instanceId, instances.data]);

  return (
    <div className="page">
      <div className="page-header">
        <h1>Download content</h1>
        <div className="actions">
          <select className="select" value={instanceId} onChange={(e) => update("instance", e.target.value)} aria-label="Instance">
            {instances.data.length === 0 && <option value="">No instances</option>}
            {instances.data.map((i) => (
              <option key={i.id} value={i.id}>
                {i.name} ({i.minecraftVersion ?? "?"}
                {i.loader ? `, ${i.loader.kind}` : ""})
              </option>
            ))}
          </select>
          <div className="segmented">
            {KINDS.map((k) => (
              <button key={k} className={k === kind ? "active" : ""} onClick={() => update("kind", k)}>
                {RESOURCE_LABELS[k].title}
              </button>
            ))}
          </div>
        </div>
      </div>
      {!instances.loaded ? (
        <Spinner />
      ) : !instance ? (
        <Empty>
          Choose an instance to install content into, or <Link to="/instances/new">create one</Link>.
        </Empty>
      ) : (
        <Browser key={`${instance.id}:${kind}`} instance={instance} kind={kind} />
      )}
    </div>
  );
}

function Browser({ instance, kind }: { instance: Instance; kind: ResourceKind }) {
  const [provider, setProvider] = useState<ModProvider>("modrinth");
  const [text, setText] = useState("");
  const query = useDebouncedValue(text, 350);
  const [sort, setSort] = useState<string>();
  const [pages, setPages] = useState<ModSearchResult[]>([]);
  const [loadingMore, setLoadingMore] = useState(false);
  const [selected, setSelected] = useState<RemoteProject>();
  const { showError } = useToasts();
  const system = useQuery(() => materialmc.system.info(), []);

  const first = useQuery(
    () => materialmc.mods.search({ instanceId: instance.id, kind, provider, query, offset: 0, ...(sort ? { sort } : {}) }),
    [instance.id, kind, provider, query, sort],
  );
  useEffect(() => setPages(first.data ? [first.data] : []), [first.data]);

  const projects = pages.flatMap((p) => p.projects);
  const sortingMethods = first.data?.sortingMethods ?? [];
  const lastPage = pages[pages.length - 1];

  const loadMore = async () => {
    setLoadingMore(true);
    try {
      const page = await materialmc.mods.search({
        instanceId: instance.id,
        kind,
        provider,
        query,
        offset: projects.length,
        ...(sort ? { sort } : {}),
      });
      setPages((p) => [...p, page]);
    } catch (e) {
      showError(e, "Search failed");
    } finally {
      setLoadingMore(false);
    }
  };

  return (
    <div className="stack">
      <div className="row">
        <div className="segmented">
          <button className={provider === "modrinth" ? "active" : ""} onClick={() => setProvider("modrinth")}>
            Modrinth
          </button>
          <button
            className={provider === "curseforge" ? "active" : ""}
            disabled={system.data ? !system.data.capabilities.curseforge : false}
            title={system.data && !system.data.capabilities.curseforge ? "This build has no CurseForge API key" : undefined}
            onClick={() => setProvider("curseforge")}
          >
            CurseForge
          </button>
        </div>
        <input className="input grow" placeholder={`Search ${RESOURCE_LABELS[kind].title.toLowerCase()}…`} value={text} onChange={(e) => setText(e.target.value)} autoFocus />
        {sortingMethods.length > 0 && (
          <select className="select" value={sort ?? sortingMethods[0]?.id} onChange={(e) => setSort(e.target.value)} aria-label="Sort">
            {sortingMethods.map((m) => (
              <option key={m.id} value={m.id}>
                {m.name}
              </option>
            ))}
          </select>
        )}
      </div>

      {first.error ? (
        <ErrorBanner error={first.error} onRetry={first.reload} />
      ) : first.loading && pages.length === 0 ? (
        <Spinner label="Searching…" />
      ) : projects.length === 0 ? (
        <Empty>No results.</Empty>
      ) : (
        <div className="card flush">
          {projects.map((p) => (
            <div key={`${p.provider}:${p.id}`} className="list-row clickable" style={{ padding: 12 }} onClick={() => setSelected(p)}>
              {p.iconUrl ? <img className="instance-icon" src={p.iconUrl} alt="" loading="lazy" /> : <div className="instance-icon" />}
              <div className="grow" style={{ minWidth: 0 }}>
                <div className="ellipsis">
                  <strong>{p.name}</strong> <span className="small muted">by {p.authors.join(", ") || "unknown"}</span>
                </div>
                <div className="small muted" style={{ display: "-webkit-box", WebkitLineClamp: 2, WebkitBoxOrient: "vertical", overflow: "hidden" }}>
                  {p.description}
                </div>
              </div>
              <button className="btn small primary">Install…</button>
            </div>
          ))}
        </div>
      )}
      {lastPage && lastPage.projects.length > 0 && (
        <div className="row" style={{ justifyContent: "center" }}>
          <button className="btn" disabled={loadingMore} onClick={() => void loadMore()}>
            {loadingMore ? "Loading…" : "Load more"}
          </button>
        </div>
      )}
      {selected && <InstallDialog instance={instance} kind={kind} project={selected} onClose={() => setSelected(undefined)} />}
    </div>
  );
}

function InstallDialog({
  instance,
  kind,
  project,
  onClose,
}: {
  instance: Instance;
  kind: ResourceKind;
  project: RemoteProject;
  onClose(): void;
}) {
  const versions = useQuery(
    () => materialmc.mods.versions({ instanceId: instance.id, kind, provider: project.provider, projectId: project.id }),
    [instance.id, kind, project.provider, project.id],
  );
  const [showAll, setShowAll] = useState(false);
  const [versionId, setVersionId] = useState<string>();
  const [busy, setBusy] = useState(false);
  const { showError, notify } = useToasts();

  const list = (versions.data ?? []).filter((v) => showAll || v.compatible);
  useEffect(() => {
    if (!versionId && list[0]) setVersionId(list[0].id);
  }, [list, versionId]);

  const install = async () => {
    if (!versionId) return;
    setBusy(true);
    try {
      const { taskId } = await materialmc.mods.install({
        instanceId: instance.id,
        kind,
        provider: project.provider,
        projectId: project.id,
        versionId,
      });
      notify(`Installing ${project.name}…`);
      onClose();
      waitForTask(taskId).then(
        () => notify(`${project.name} installed into ${instance.name}`, "success"),
        (e: unknown) => showError(e, `Installing ${project.name} failed`),
      );
    } catch (e) {
      showError(e, "Could not start the download");
      setBusy(false);
    }
  };

  return (
    <Dialog
      title={project.name}
      wide
      onClose={onClose}
      footer={
        <>
          {project.websiteUrl && (
            <button className="btn ghost" onClick={() => void materialmc.system.openUrl(project.websiteUrl)}>
              Open project page ↗
            </button>
          )}
          <div className="grow" />
          <button className="btn" onClick={onClose}>
            Cancel
          </button>
          <button className="btn primary" disabled={!versionId || busy} onClick={() => void install()}>
            Install
          </button>
        </>
      }
    >
      <p className="muted" style={{ margin: 0 }}>
        {project.description}
      </p>
      <label className="check small">
        <input type="checkbox" checked={showAll} onChange={(e) => setShowAll(e.target.checked)} /> Show versions for other game
        versions / loaders
      </label>
      {versions.error ? (
        <ErrorBanner error={versions.error} onRetry={versions.reload} />
      ) : !versions.data ? (
        <Spinner label="Loading versions…" />
      ) : list.length === 0 ? (
        <div className="banner warn">
          No version is compatible with Minecraft {instance.minecraftVersion}
          {instance.loader ? ` / ${instance.loader.kind}` : ""}.
        </div>
      ) : (
        <div className="card flush" style={{ maxHeight: 360, overflow: "auto" }}>
          {list.map((v) => (
            <label key={v.id} className={`list-row clickable${versionId === v.id ? " selected" : ""}`} style={{ padding: 10 }}>
              <input type="radio" name="version" checked={versionId === v.id} onChange={() => setVersionId(v.id)} />
              <div className="grow" style={{ minWidth: 0 }}>
                <div className="ellipsis">
                  <strong>{v.name || v.versionNumber}</strong> <span className="small muted">{v.fileName}</span>
                </div>
                <div className="small muted ellipsis">
                  {v.gameVersions.slice(0, 6).join(", ")}
                  {v.gameVersions.length > 6 ? "…" : ""} · {v.loaders.join(", ")}
                </div>
              </div>
              {v.type && <span className={`chip ${v.type.toLowerCase() === "release" ? "ok" : "warn"}`}>{v.type}</span>}
              {!v.compatible && <span className="chip err">Incompatible</span>}
            </label>
          ))}
        </div>
      )}
    </Dialog>
  );
}
