import { useEffect, useState, type ReactNode } from "react";
import { materialmc } from "../api/client";
import { ErrorBanner, QueryView, Spinner } from "../components/common";
import { useToasts } from "../components/Toasts";
import { useEvent, useQuery } from "../hooks/useApi";
import type { LauncherSettingKey, LauncherSettings } from "../types/settings";

type BoolKey = { [K in LauncherSettingKey]: LauncherSettings[K] extends boolean ? K : never }[LauncherSettingKey];
type NumKey = { [K in LauncherSettingKey]: LauncherSettings[K] extends number ? K : never }[LauncherSettingKey];
type StrKey = { [K in LauncherSettingKey]: LauncherSettings[K] extends string ? K : never }[LauncherSettingKey];

export function SettingsPage() {
  const query = useQuery(() => materialmc.settings.get(), []);
  useEvent("settings.changed", query.reload);
  return (
    <div className="page">
      <div className="page-header">
        <h1>Settings</h1>
      </div>
      <QueryView query={query}>{(s) => <SettingsForm initial={s} />}</QueryView>
    </div>
  );
}

function SettingsForm({ initial }: { initial: LauncherSettings }) {
  const [draft, setDraft] = useState(initial);
  const [saving, setSaving] = useState(false);
  const { showError, notify } = useToasts();
  useEffect(() => setDraft(initial), [initial]);

  const changed = (Object.keys(draft) as LauncherSettingKey[]).filter((k) => draft[k] !== initial[k]);
  const set = <K extends LauncherSettingKey>(k: K, v: LauncherSettings[K]) => setDraft((d) => ({ ...d, [k]: v }));

  const bool = (k: BoolKey, label: string) => (
    <label className="check">
      <input type="checkbox" checked={draft[k]} onChange={(e) => set(k, e.target.checked)} /> {label}
    </label>
  );
  const num = (k: NumKey, label: string, hint?: string) => (
    <label className="field">
      <span>{label}</span>
      <input className="input" type="number" min={0} value={draft[k]} onChange={(e) => set(k, Number(e.target.value))} />
      {hint && <span className="hint">{hint}</span>}
    </label>
  );
  const str = (k: StrKey, label: string, hint?: string, mono = false) => (
    <label className="field">
      <span>{label}</span>
      <input className={`input${mono ? " mono" : ""}`} value={draft[k]} onChange={(e) => set(k, e.target.value)} />
      {hint && <span className="hint">{hint}</span>}
    </label>
  );

  const save = async () => {
    setSaving(true);
    try {
      const values: Partial<LauncherSettings> = {};
      for (const k of changed) (values as Record<string, unknown>)[k] = draft[k];
      const updated = await materialmc.settings.set({ values });
      setDraft(updated);
      notify("Settings saved", "success");
    } catch (e) {
      showError(e, "Settings were not saved");
    } finally {
      setSaving(false);
    }
  };

  return (
    <div className="stack">
      <Section title="Folders">
        <div className="grid-2">
          {str("InstanceDir", "Instances folder", "Relative paths are resolved against the launcher data folder.")}
          {str("DownloadsDir", "Downloads folder")}
        </div>
      </Section>

      <Section title="Java">
        <JavaPicker value={draft.JavaPath} onChange={(v) => set("JavaPath", v)} />
        <div className="grid-2">
          {num("MinMemAlloc", "Minimum memory (MiB)")}
          {num("MaxMemAlloc", "Maximum memory (MiB)")}
          {num("PermGen", "PermGen (MiB)", "Only used by Java 7 and older.")}
        </div>
        <label className="field">
          <span>JVM arguments</span>
          <textarea className="textarea mono" rows={3} value={draft.JvmArgs} onChange={(e) => set("JvmArgs", e.target.value)} />
        </label>
        {bool("AutomaticJavaSwitch", "Automatically switch to a compatible Java version")}
        {bool("AutomaticJavaDownload", "Automatically download Java when needed")}
        {bool("IgnoreJavaCompatibility", "Skip Java compatibility checks")}
      </Section>

      <Section title="Game window">
        {bool("LaunchMaximized", "Start maximized")}
        <div className="grid-2">
          {num("MinecraftWinWidth", "Width")}
          {num("MinecraftWinHeight", "Height")}
        </div>
      </Section>

      <Section title="Console & behaviour">
        {bool("ShowConsole", "Show the console when a game launches")}
        {bool("AutoCloseConsole", "Close the console when the game exits normally")}
        {bool("ShowConsoleOnError", "Show the console when the game crashes")}
        {num("ConsoleMaxLines", "Console line limit")}
        {bool("CloseAfterLaunch", "Hide the launcher while the game is running")}
        {bool("QuitAfterGameStop", "Quit the launcher after the game stops")}
        {bool("RecordGameTime", "Record play time")}
        {bool("ShowGameTime", "Show play time")}
      </Section>

      <Section title="Custom commands" note="Commands run on your system. Changes must be confirmed in a system dialog.">
        {str("PreLaunchCommand", "Pre-launch command", undefined, true)}
        {str("WrapperCommand", "Wrapper command", undefined, true)}
        {str("PostExitCommand", "Post-exit command", undefined, true)}
      </Section>

      <Section title="Network">
        <div className="grid-2">
          {num("NumberOfConcurrentDownloads", "Concurrent downloads")}
          {num("NumberOfConcurrentTasks", "Concurrent tasks")}
          {num("RequestTimeout", "Request timeout (seconds)")}
        </div>
        {/* TODO(webui): proxy settings need Application::updateProxySettings wired through SettingsApi. */}
      </Section>

      <div className="row" style={{ position: "sticky", bottom: 0, background: "var(--bg)", padding: "12px 0" }}>
        <span className="grow muted small">{changed.length > 0 ? `${changed.length} unsaved change(s)` : "All changes saved"}</span>
        <button className="btn" disabled={changed.length === 0 || saving} onClick={() => setDraft(initial)}>
          Revert
        </button>
        <button className="btn primary" disabled={changed.length === 0 || saving} onClick={() => void save()}>
          Save
        </button>
      </div>
    </div>
  );
}

function Section({ title, note, children }: { title: string; note?: string; children: ReactNode }) {
  return (
    <section className="card stack">
      <h2>{title}</h2>
      {note && <div className="small muted">{note}</div>}
      {children}
    </section>
  );
}

function JavaPicker({ value, onChange }: { value: string; onChange(v: string): void }) {
  const [reload, setReload] = useState(0);
  const list = useQuery(() => materialmc.java.list({ forceReload: reload > 0 }), [reload]);
  return (
    <div className="stack">
      <label className="field">
        <span>Java executable</span>
        <div className="row">
          <input className="input grow mono" value={value} onChange={(e) => onChange(e.target.value)} placeholder="Auto-detect" />
          <button className="btn" onClick={() => setReload((n) => n + 1)} disabled={list.loading}>
            Detect
          </button>
        </div>
      </label>
      {list.error ? (
        <ErrorBanner error={list.error} onRetry={list.reload} />
      ) : list.loading ? (
        <Spinner label="Detecting Java installations…" />
      ) : (
        (list.data ?? []).length > 0 && (
          <div className="card flush" style={{ maxHeight: 220, overflow: "auto" }}>
            {(list.data ?? []).map((j) => (
              <label key={j.path} className={`list-row clickable${j.path === value ? " selected" : ""}`} style={{ padding: 8 }}>
                <input type="radio" name="java" checked={j.path === value} onChange={() => onChange(j.path)} />
                <strong>{j.version}</strong>
                <span className="chip">{j.architecture}</span>
                {j.recommended && <span className="chip ok">Recommended</span>}
                {j.isManaged && <span className="chip info">Managed</span>}
                <span className="small muted mono ellipsis grow">{j.path}</span>
              </label>
            ))}
          </div>
        )
      )}
    </div>
  );
}
