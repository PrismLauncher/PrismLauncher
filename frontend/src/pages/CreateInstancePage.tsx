import { useEffect, useMemo, useState } from "react";
import { useNavigate } from "react-router-dom";
import { materialmc } from "../api/client";
import { ErrorBanner, Spinner } from "../components/common";
import { formatDate, LOADER_NAMES } from "../components/format";
import { useToasts } from "../components/Toasts";
import { VirtualList } from "../components/VirtualList";
import { useQuery } from "../hooks/useApi";
import { waitForTask } from "../hooks/useTaskCompletion";
import type { LoaderKind } from "../types/instances";
import type { LoaderVersion, MinecraftVersion } from "../types/versions";

const VERSION_TYPES = [
  { id: "release", label: "Releases" },
  { id: "snapshot", label: "Snapshots" },
  { id: "old_beta", label: "Beta" },
  { id: "old_alpha", label: "Alpha" },
  { id: "experiment", label: "Experiments" },
] as const;

const LOADERS: Array<LoaderKind | null> = [null, "fabric", "quilt", "neoforge", "forge", "liteloader"];

function compareVersions(a: string, b: string): number {
  return a.localeCompare(b, undefined, { numeric: true });
}

/** Mirrors the Qt CustomPage rule: Fabric/Quilt only exist for 1.14+. */
function loaderAvailable(loader: LoaderKind, mc: MinecraftVersion | undefined): boolean {
  if (!mc) return false;
  if (loader === "fabric" || loader === "quilt") return mc.type !== "release" || compareVersions(mc.id, "1.14") >= 0;
  return true;
}

export function CreateInstancePage() {
  const navigate = useNavigate();
  const { showError, notify } = useToasts();
  const [types, setTypes] = useState<Set<string>>(new Set(["release"]));
  const [search, setSearch] = useState("");
  const [selected, setSelected] = useState<MinecraftVersion>();
  const [loader, setLoader] = useState<LoaderKind | null>(null);
  const [loaderVersion, setLoaderVersion] = useState<LoaderVersion>();
  const [name, setName] = useState("");
  const [nameTouched, setNameTouched] = useState(false);
  const [group, setGroup] = useState("");
  const [busy, setBusy] = useState(false);
  const [reloadMc, setReloadMc] = useState(0);

  const versions = useQuery(() => materialmc.versions.minecraft({ forceReload: reloadMc > 0 }), [reloadMc]);
  const groups = useQuery(() => materialmc.instances.groups(), []);
  const loaderVersions = useQuery<LoaderVersion[] | null>(
    () =>
      loader && selected
        ? materialmc.versions.loaders({ loader, minecraftVersion: selected.id })
        : Promise.resolve(null),
    [loader, selected?.id],
  );

  const filtered = useMemo(() => {
    const q = search.trim();
    return (versions.data ?? []).filter((v) => types.has(v.type) && (!q || v.id.includes(q)));
  }, [versions.data, types, search]);

  // Select the recommended release once the list arrives.
  useEffect(() => {
    if (!selected && versions.data) {
      setSelected(versions.data.find((v) => v.recommended) ?? versions.data.find((v) => v.type === "release"));
    }
  }, [versions.data, selected]);

  useEffect(() => {
    const list = loaderVersions.data;
    setLoaderVersion(list ? (list.find((v) => v.recommended) ?? list[0]) : undefined);
  }, [loaderVersions.data]);

  useEffect(() => {
    if (nameTouched || !selected) return;
    setName(loader ? `${selected.id} ${LOADER_NAMES[loader]}` : selected.id);
  }, [selected, loader, nameTouched]);

  const loaderBlocked = loader !== null && (!loaderVersion || loaderVersions.loading);
  const canCreate = !!selected && name.trim() !== "" && !loaderBlocked && !busy;

  const create = async () => {
    if (!selected) return;
    setBusy(true);
    try {
      const { taskId } = await materialmc.instances.create({
        name: name.trim(),
        group: group.trim() || null,
        minecraftVersion: selected.id,
        loader: loader && loaderVersion ? { kind: loader, version: loaderVersion.version } : null,
      });
      notify(`Creating ${name.trim()}…`);
      navigate("/instances");
      waitForTask(taskId).then(
        () => notify(`${name.trim()} is ready`, "success"),
        (e: unknown) => showError(e, "Instance creation failed"),
      );
    } catch (e) {
      showError(e, "Could not create the instance");
      setBusy(false);
    }
  };

  return (
    <div className="page">
      <div className="page-header">
        <h1>New instance</h1>
        <div className="actions">
          <button className="btn" onClick={() => navigate(-1)}>
            Cancel
          </button>
          <button className="btn primary" disabled={!canCreate} onClick={() => void create()}>
            Create
          </button>
        </div>
      </div>

      <div className="card grid-2">
        <label className="field">
          <span>Name</span>
          <input
            className="input"
            value={name}
            onChange={(e) => {
              setName(e.target.value);
              setNameTouched(true);
            }}
          />
        </label>
        <label className="field">
          <span>Group</span>
          <input className="input" list="instance-groups" value={group} onChange={(e) => setGroup(e.target.value)} placeholder="Ungrouped" />
          <datalist id="instance-groups">
            {(groups.data ?? []).map((g) => (
              <option key={g} value={g} />
            ))}
          </datalist>
        </label>
      </div>

      <div className="grid-2" style={{ alignItems: "start" }}>
        <section className="card stack">
          <div className="row">
            <h2 className="grow">Minecraft version</h2>
            <button className="btn small" onClick={() => setReloadMc((n) => n + 1)} disabled={versions.loading}>
              Refresh
            </button>
          </div>
          <div className="row">
            {VERSION_TYPES.map((t) => (
              <label key={t.id} className="check small">
                <input
                  type="checkbox"
                  checked={types.has(t.id)}
                  onChange={(e) => {
                    const next = new Set(types);
                    if (e.target.checked) next.add(t.id);
                    else next.delete(t.id);
                    setTypes(next);
                  }}
                />
                {t.label}
              </label>
            ))}
          </div>
          <input className="input" placeholder="Filter versions…" value={search} onChange={(e) => setSearch(e.target.value)} />
          {versions.error ? (
            <ErrorBanner error={versions.error} onRetry={versions.reload} />
          ) : versions.loading && !versions.data ? (
            <Spinner label="Fetching version list…" />
          ) : (
            <VirtualList
              items={filtered}
              rowHeight={40}
              style={{ height: 360, border: "1px solid var(--border)", borderRadius: 8 }}
              getKey={(v) => v.id}
              renderRow={(v) => (
                <div
                  className={`list-row clickable${selected?.id === v.id ? " selected" : ""}`}
                  style={{ height: 40 }}
                  onClick={() => setSelected(v)}
                >
                  <strong className="grow">{v.id}</strong>
                  {v.recommended && <span className="chip ok">Recommended</span>}
                  <span className="chip">{v.type}</span>
                  <span className="small muted">{v.releaseTime ? formatDate(v.releaseTime).split(",")[0] : ""}</span>
                </div>
              )}
            />
          )}
        </section>

        <section className="card stack">
          <h2>Mod loader</h2>
          <div className="segmented" role="radiogroup" style={{ flexWrap: "wrap" }}>
            {LOADERS.map((l) => (
              <button
                key={l ?? "none"}
                role="radio"
                aria-checked={loader === l}
                className={loader === l ? "active" : ""}
                disabled={l !== null && !loaderAvailable(l, selected)}
                onClick={() => setLoader(l)}
              >
                {l ? LOADER_NAMES[l] : "None"}
              </button>
            ))}
          </div>
          {loader === null ? (
            <div className="muted">Vanilla Minecraft without a mod loader.</div>
          ) : loaderVersions.error ? (
            <ErrorBanner error={loaderVersions.error} onRetry={loaderVersions.reload} />
          ) : loaderVersions.loading ? (
            <Spinner label={`Fetching ${LOADER_NAMES[loader]} versions…`} />
          ) : (loaderVersions.data ?? []).length === 0 ? (
            <div className="banner warn">
              {LOADER_NAMES[loader]} has no versions for Minecraft {selected?.id}.
            </div>
          ) : (
            <VirtualList
              items={loaderVersions.data ?? []}
              rowHeight={40}
              style={{ height: 360, border: "1px solid var(--border)", borderRadius: 8 }}
              getKey={(v) => v.version}
              renderRow={(v) => (
                <div
                  className={`list-row clickable${loaderVersion?.version === v.version ? " selected" : ""}`}
                  style={{ height: 40 }}
                  onClick={() => setLoaderVersion(v)}
                >
                  <strong className="grow ellipsis">{v.version}</strong>
                  {v.recommended && <span className="chip ok">Recommended</span>}
                  {v.type && v.type !== "release" && <span className="chip">{v.type}</span>}
                  <span className="small muted">{v.releaseTime ? formatDate(v.releaseTime) : ""}</span>
                </div>
              )}
            />
          )}
        </section>
      </div>
    </div>
  );
}
