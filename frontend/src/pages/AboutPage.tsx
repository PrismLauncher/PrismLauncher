import { materialmc } from "../api/client";
import { QueryView } from "../components/common";
import { useQuery } from "../hooks/useApi";

export function AboutPage() {
  const info = useQuery(() => materialmc.system.info(), []);
  return (
    <div className="page">
      <div className="page-header">
        <h1>About</h1>
      </div>
      <QueryView query={info}>
        {(i) => (
          <>
            <section className="card stack">
              <div className="row">
                <div className="brand-mark" style={{ width: 48, height: 48, borderRadius: 12, background: "linear-gradient(135deg, var(--primary), #3f7fbf)" }} />
                <div>
                  <h2>{i.displayName}</h2>
                  <div className="muted">Version {i.version}</div>
                </div>
              </div>
              <p className="muted" style={{ margin: 0 }}>
                A Minecraft launcher based on Prism Launcher. The interface is a React application rendered by the system WebView; all
                launcher logic runs in the C++ core.
              </p>
            </section>
            <section className="card">
              <table className="table">
                <tbody>
                  <Row k="Git commit" v={<code>{i.gitCommit || "unknown"}</code>} />
                  <Row k="Build platform" v={i.buildPlatform} />
                  <Row k="Operating system" v={i.os} />
                  <Row k="Qt" v={i.qtVersion} />
                  <Row k="WebView" v={i.webViewEngine} />
                  <Row k="Portable" v={i.isPortable ? "yes" : "no"} />
                  <Row k="Data folder" v={<code className="small">{i.dataPath}</code>} />
                  <Row k="Developer mode" v={i.devMode ? "yes" : "no"} />
                </tbody>
              </table>
            </section>
            <section className="card row">
              {Object.entries(i.urls)
                .filter(([, url]) => url)
                .map(([name, url]) => (
                  <button key={name} className="btn" onClick={() => void materialmc.system.openUrl(url)}>
                    {name[0]?.toUpperCase()}
                    {name.slice(1)} ↗
                  </button>
                ))}
              <button className="btn" onClick={() => void materialmc.system.openFolder({ target: "data" })}>
                Open data folder
              </button>
              <button className="btn" onClick={() => void materialmc.system.openFolder({ target: "logs" })}>
                Open logs folder
              </button>
            </section>
            <p className="small muted">Licensed under the GNU General Public License v3. Not affiliated with Mojang or Microsoft.</p>
          </>
        )}
      </QueryView>
    </div>
  );
}

function Row({ k, v }: { k: string; v: React.ReactNode }) {
  return (
    <tr>
      <td className="muted" style={{ width: 200 }}>
        {k}
      </td>
      <td>{v}</td>
    </tr>
  );
}
