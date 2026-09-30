import { useState } from "react";
import { NavLink, Outlet, useLocation } from "react-router-dom";
import { materialmc } from "../api/client";
import { useAccounts, useTasks } from "../hooks/stores";

const NAV = [
  { to: "/", label: "Home", icon: "⌂", end: true },
  { to: "/instances", label: "Instances", icon: "▦" },
  { to: "/mods", label: "Mods", icon: "✚" },
  { to: "/worlds", label: "Worlds", icon: "◍" },
  { to: "/downloads", label: "Downloads", icon: "⇩" },
  { to: "/accounts", label: "Accounts", icon: "☺" },
  { to: "/console", label: "Console", icon: "›_" },
  { to: "/settings", label: "Settings", icon: "⚙" },
  { to: "/about", label: "About", icon: "ⓘ" },
] as const;

export function Layout() {
  const [open, setOpen] = useState(false);
  const location = useLocation();
  const tasks = useTasks();
  const accounts = useAccounts();
  const running = tasks.data.filter((t) => t.state === "running").length;
  const defaultAccount = accounts.data.find((a) => a.isDefault);

  return (
    <div className="app">
      <header className="topbar">
        <button className="btn icon-only ghost menu-toggle" aria-label="Toggle navigation" onClick={() => setOpen((o) => !o)}>
          ☰
        </button>
        <div className="brand">
          <div className="brand-mark" aria-hidden />
          MaterialMC
        </div>
        <div className="spacer" />
        {running > 0 && (
          <NavLink to="/downloads" className="chip info" style={{ textDecoration: "none" }}>
            <span className="spinner" style={{ width: 12, height: 12 }} /> {running} task{running > 1 ? "s" : ""}
          </NavLink>
        )}
        <NavLink to="/accounts" className="row" style={{ textDecoration: "none", color: "inherit" }}>
          {defaultAccount ? (
            <>
              <img src={defaultAccount.faceUrl} alt="" width={28} height={28} style={{ borderRadius: 6, imageRendering: "pixelated" }} />
              <span className="ellipsis" style={{ maxWidth: 160 }}>
                {defaultAccount.profileName || "Account"}
              </span>
            </>
          ) : (
            <span className="chip warn">No default account</span>
          )}
        </NavLink>
      </header>
      <nav className={`sidebar${open ? " open" : ""}`} onClick={() => setOpen(false)}>
        {NAV.map((item) => (
          <NavLink key={item.to} to={item.to} end={"end" in item ? item.end : false}>
            <span className="icon" aria-hidden>
              {item.icon}
            </span>
            {item.label}
            {item.to === "/downloads" && running > 0 && <span className="chip info badge">{running}</span>}
          </NavLink>
        ))}
      </nav>
      <main className="main" key={location.pathname.split("/")[1]}>
        {!materialmc.connected && (
          <div className="banner error" style={{ marginBottom: 16 }}>
            <strong>Backend not connected.</strong> This UI only works inside the MaterialMC application (or a dev
            session started with <code>MATERIALMC_DEV_URL</code>). No data is shown without it.
          </div>
        )}
        <Outlet />
      </main>
    </div>
  );
}
