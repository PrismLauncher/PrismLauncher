import { useEffect, useState } from "react";
import { materialmc } from "./api/client";
import type { SystemInfo } from "./types/system";
import { describeError } from "./components/Toasts";

/** Minimal shell: shows whether the native bridge is connected. Screens are added on top of this. */
export function App() {
  const [info, setInfo] = useState<SystemInfo>();
  const [error, setError] = useState<string>();
  useEffect(() => {
    materialmc.system.info().then(setInfo, (e: unknown) => setError(describeError(e).message));
  }, []);
  return (
    <main className="main">
      <h1>MaterialMC</h1>
      {info ? <p>{info.displayName} {info.version} — {info.webViewEngine}</p> : <p className="muted">{error ?? "Connecting…"}</p>}
    </main>
  );
}
