import { useEffect, useState } from "react";
import { materialmc } from "../api/client";
import { useAccounts } from "../hooks/stores";
import { useQuery } from "../hooks/useApi";
import type { InstanceSettings } from "../types/instances";
import { QueryView } from "./common";
import { useToasts } from "./Toasts";

export function InstanceSettingsView({ instanceId }: { instanceId: string }) {
  const query = useQuery(() => materialmc.instances.getSettings(instanceId), [instanceId]);
  return <QueryView query={query}>{(settings) => <Form instanceId={instanceId} initial={settings} />}</QueryView>;
}

function Form({ instanceId, initial }: { instanceId: string; initial: InstanceSettings }) {
  const [draft, setDraft] = useState(initial);
  const [saving, setSaving] = useState(false);
  const { showError, notify } = useToasts();
  const accounts = useAccounts();
  useEffect(() => setDraft(initial), [initial]);

  const set = <K extends keyof InstanceSettings>(key: K, value: InstanceSettings[K]) => setDraft((d) => ({ ...d, [key]: value }));
  const changed = (Object.keys(draft) as Array<keyof InstanceSettings>).filter((k) => draft[k] !== initial[k]);

  const save = async () => {
    setSaving(true);
    try {
      const patch: Partial<InstanceSettings> = {};
      for (const k of changed) (patch as Record<string, unknown>)[k] = draft[k];
      const updated = await materialmc.instances.setSettings({ id: instanceId, settings: patch });
      setDraft(updated);
      notify("Instance settings saved", "success");
    } catch (e) {
      showError(e, "Could not save settings");
    } finally {
      setSaving(false);
    }
  };

  const num = (key: "minMemAlloc" | "maxMemAlloc" | "minecraftWinWidth" | "minecraftWinHeight") => (
    <input className="input" type="number" min={0} value={draft[key]} onChange={(e) => set(key, Number(e.target.value))} />
  );

  return (
    <div className="stack">
      <section className="card stack">
        <label className="check">
          <input type="checkbox" checked={draft.overrideMemory} onChange={(e) => set("overrideMemory", e.target.checked)} />
          <h3>Memory</h3>
        </label>
        <fieldset disabled={!draft.overrideMemory} className="grid-2" style={{ border: "none", padding: 0, margin: 0 }}>
          <label className="field">
            <span>Minimum (MiB)</span>
            {num("minMemAlloc")}
          </label>
          <label className="field">
            <span>Maximum (MiB)</span>
            {num("maxMemAlloc")}
          </label>
        </fieldset>
      </section>

      <section className="card stack">
        <label className="check">
          <input type="checkbox" checked={draft.overrideJavaLocation} onChange={(e) => set("overrideJavaLocation", e.target.checked)} />
          <h3>Java installation</h3>
        </label>
        <label className="field">
          <span>Java executable</span>
          <input className="input mono" disabled={!draft.overrideJavaLocation} value={draft.javaPath} onChange={(e) => set("javaPath", e.target.value)} />
          <span className="hint">Changing this requires confirmation in a system dialog.</span>
        </label>
        <label className="check">
          <input type="checkbox" checked={draft.overrideJavaArgs} onChange={(e) => set("overrideJavaArgs", e.target.checked)} />
          Custom JVM arguments
        </label>
        <textarea className="textarea mono" rows={3} disabled={!draft.overrideJavaArgs} value={draft.jvmArgs} onChange={(e) => set("jvmArgs", e.target.value)} />
      </section>

      <section className="card stack">
        <label className="check">
          <input type="checkbox" checked={draft.overrideWindow} onChange={(e) => set("overrideWindow", e.target.checked)} />
          <h3>Game window</h3>
        </label>
        <fieldset disabled={!draft.overrideWindow} className="stack" style={{ border: "none", padding: 0, margin: 0 }}>
          <label className="check">
            <input type="checkbox" checked={draft.launchMaximized} onChange={(e) => set("launchMaximized", e.target.checked)} /> Start maximized
          </label>
          <div className="grid-2">
            <label className="field">
              <span>Width</span>
              {num("minecraftWinWidth")}
            </label>
            <label className="field">
              <span>Height</span>
              {num("minecraftWinHeight")}
            </label>
          </div>
        </fieldset>
      </section>

      <section className="card stack">
        <h3>Launch</h3>
        <label className="check">
          <input type="checkbox" checked={draft.useAccountForInstance} onChange={(e) => set("useAccountForInstance", e.target.checked)} />
          Use a specific account for this instance
        </label>
        <select
          className="select"
          disabled={!draft.useAccountForInstance}
          value={draft.instanceAccountId}
          onChange={(e) => set("instanceAccountId", e.target.value)}
        >
          <option value="">Default account</option>
          {accounts.data.map((a) => (
            <option key={a.id} value={a.profileId}>
              {a.profileName} ({a.type === "msa" ? "Microsoft" : "Offline"})
            </option>
          ))}
        </select>
        <label className="check">
          <input type="checkbox" checked={draft.joinServerOnLaunch} onChange={(e) => set("joinServerOnLaunch", e.target.checked)} />
          Join a server on launch
        </label>
        <input
          className="input"
          placeholder="play.example.org:25565"
          disabled={!draft.joinServerOnLaunch}
          value={draft.joinServerOnLaunchAddress}
          onChange={(e) => set("joinServerOnLaunchAddress", e.target.value)}
        />
      </section>

      <div className="row">
        <div className="grow" />
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
