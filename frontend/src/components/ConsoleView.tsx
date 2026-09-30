import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { materialmc } from "../api/client";
import type { ConsoleLogChunk, LogLevel, LogLine } from "../types/console";
import { ErrorBanner } from "./common";
import { useToasts } from "./Toasts";
import { VirtualList, type VirtualListHandle } from "./VirtualList";

const MAX_LINES = 100_000;

const LEVEL_GROUPS = [
  { id: "launcher", label: "Launcher", levels: ["launcher"] },
  { id: "info", label: "Info", levels: ["info", "message", "stdout", "unknown"] },
  { id: "debug", label: "Debug", levels: ["debug", "trace"] },
  { id: "warning", label: "Warnings", levels: ["warning"] },
  { id: "error", label: "Errors", levels: ["error", "fatal", "stderr"] },
] as const satisfies ReadonlyArray<{ id: string; label: string; levels: readonly LogLevel[] }>;

type GroupId = (typeof LEVEL_GROUPS)[number]["id"];

interface Source {
  /** Fetches lines with n >= from. */
  fetch(from: number): Promise<Pick<ConsoleLogChunk, "lines" | "next" | "available">>;
  /** Subscribes to pushed lines, returns an unsubscribe function. */
  subscribe(push: (lines: LogLine[]) => void, reset: () => void): () => void;
  fileName: string;
  /** False when the source has no event stream and must be refreshed manually. */
  live: boolean;
}

function highlight(text: string, needle: string) {
  if (!needle) return text;
  const lower = text.toLowerCase();
  const parts: Array<string | React.ReactElement> = [];
  let index = 0;
  let found = lower.indexOf(needle, index);
  while (found !== -1) {
    parts.push(text.slice(index, found), <mark key={found}>{text.slice(found, found + needle.length)}</mark>);
    index = found + needle.length;
    found = lower.indexOf(needle, index);
  }
  parts.push(text.slice(index));
  return parts;
}

function useLogSource(source: Source) {
  const [lines, setLines] = useState<LogLine[]>([]);
  const [available, setAvailable] = useState(true);
  const [error, setError] = useState<unknown>(null);
  const next = useRef(0);
  const buffer = useRef<LogLine[]>([]);

  const append = useCallback((incoming: LogLine[]) => {
    const fresh = incoming.filter((l) => l.n >= next.current);
    if (fresh.length === 0) return;
    next.current = (fresh[fresh.length - 1]?.n ?? next.current) + 1;
    buffer.current = buffer.current.concat(fresh);
    if (buffer.current.length > MAX_LINES) buffer.current = buffer.current.slice(-MAX_LINES);
    setLines(buffer.current);
  }, []);

  const load = useCallback(async () => {
    buffer.current = [];
    next.current = 0;
    setLines([]);
    try {
      // Pull the backlog page by page; the backend caps each response at 5000 lines.
      for (;;) {
        const chunk = await source.fetch(next.current);
        setAvailable(chunk.available);
        append(chunk.lines);
        next.current = Math.max(next.current, chunk.next);
        if (chunk.lines.length < 5000) break;
      }
      setError(null);
    } catch (e) {
      setError(e);
    }
  }, [source, append]);

  useEffect(() => {
    void load();
    return source.subscribe(append, () => void load());
  }, [source, load, append]);

  return { lines, available, error, reload: load };
}

export function ConsoleView({ source, height = "calc(100vh - 300px)" }: { source: Source; height?: string }) {
  const { lines, available, error, reload } = useLogSource(source);
  const { showError, notify } = useToasts();
  const [enabled, setEnabled] = useState<Set<GroupId>>(new Set(LEVEL_GROUPS.map((g) => g.id)));
  const [search, setSearch] = useState("");
  const [autoscroll, setAutoscroll] = useState(true);
  const [clearedBefore, setClearedBefore] = useState(-1);
  const list = useRef<VirtualListHandle>(null);

  const allowed = useMemo(() => {
    const set = new Set<LogLevel>();
    for (const g of LEVEL_GROUPS) if (enabled.has(g.id)) g.levels.forEach((l) => set.add(l));
    return set;
  }, [enabled]);

  const needle = search.trim().toLowerCase();
  const visible = useMemo(
    () => lines.filter((l) => l.n > clearedBefore && allowed.has(l.level) && (!needle || l.text.toLowerCase().includes(needle))),
    [lines, allowed, needle, clearedBefore],
  );

  useEffect(() => {
    if (autoscroll) list.current?.scrollToBottom();
  }, [visible, autoscroll]);

  const text = () => visible.map((l) => l.text).join("\n");

  return (
    <div className="stack">
      <div className="row">
        <input className="input grow" placeholder="Search log…" value={search} onChange={(e) => setSearch(e.target.value)} />
        {LEVEL_GROUPS.map((g) => (
          <label key={g.id} className="check small">
            <input
              type="checkbox"
              checked={enabled.has(g.id)}
              onChange={(e) => {
                const next = new Set(enabled);
                if (e.target.checked) next.add(g.id);
                else next.delete(g.id);
                setEnabled(next);
              }}
            />
            {g.label}
          </label>
        ))}
      </div>
      <div className="row">
        <label className="check small">
          <input type="checkbox" checked={autoscroll} onChange={(e) => setAutoscroll(e.target.checked)} /> Autoscroll
        </label>
        <span className="grow small muted">
          {visible.length.toLocaleString()} / {lines.length.toLocaleString()} lines
        </span>
        {!source.live && (
          <button className="btn small" onClick={() => void reload()}>
            Refresh
          </button>
        )}
        <button className="btn small" onClick={() => setClearedBefore(lines[lines.length - 1]?.n ?? -1)} disabled={lines.length === 0}>
          Clear
        </button>
        <button
          className="btn small"
          disabled={visible.length === 0}
          onClick={() => materialmc.system.copyText(text()).then(() => notify("Copied to clipboard", "success"), (e: unknown) => showError(e))}
        >
          Copy
        </button>
        <button
          className="btn small"
          disabled={visible.length === 0}
          onClick={() =>
            materialmc.system
              .saveText({ suggestedName: source.fileName, content: text() })
              .then((r) => r.saved && notify("Log saved", "success"), (e: unknown) => showError(e, "Save failed"))
          }
        >
          Save…
        </button>
      </div>
      {error ? <ErrorBanner error={error} onRetry={() => void reload()} /> : null}
      {!available && lines.length === 0 ? (
        <div className="console empty" style={{ height }}>
          No output in this session yet. Launch the instance to follow its log live; older logs are on the Logs tab.
        </div>
      ) : (
        <VirtualList
          ref={list}
          className="console"
          style={{ height, minHeight: 240 }}
          items={visible}
          rowHeight={20}
          getKey={(l) => l.n}
          onScroll={(atBottom) => setAutoscroll(atBottom)}
          renderRow={(l) => (
            <div className={`line lvl-${l.level}`} title={l.text}>
              {highlight(l.text, needle)}
            </div>
          )}
        />
      )}
    </div>
  );
}

/** Log source for a Minecraft instance: pulls the backlog, then follows `minecraft.log` events. */
export function instanceLogSource(instanceId: string): Source {
  return {
    fileName: `${instanceId}-console.log`,
    live: true,
    fetch: (from) => materialmc.console.get({ instanceId, from }),
    subscribe(push, reset) {
      const offs = [
        materialmc.on("minecraft.log", (e) => e.instanceId === instanceId && push(e.lines)),
        materialmc.on("instance.launching", (e) => e.instanceId === instanceId && reset()),
      ];
      return () => offs.forEach((off) => off());
    },
  };
}

/** Log source for the launcher's own log. It has no live event stream; the view refetches on demand. */
export const launcherLogSource: Source = {
  fileName: "materialmc-launcher.log",
  live: false,
  fetch: (from) => materialmc.console.launcherLog({ from }),
  subscribe: () => () => undefined,
};
