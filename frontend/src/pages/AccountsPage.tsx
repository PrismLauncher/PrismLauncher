import { useRef, useState } from "react";
import { materialmc } from "../api/client";
import { ConfirmDialog, Dialog } from "../components/Dialog";
import { Empty, ErrorBanner, Spinner } from "../components/common";
import { MenuButton } from "../components/Menu";
import { describeError, useToasts } from "../components/Toasts";
import { useAccounts } from "../hooks/stores";
import { useEvent, useQuery } from "../hooks/useApi";
import type { Account, AccountState } from "../types/accounts";
import type { LoginPromptEvent } from "../types/events";

const STATE_CHIP: Record<AccountState, { label: string; cls: string }> = {
  online: { label: "Ready", cls: "ok" },
  offline: { label: "Offline", cls: "" },
  unchecked: { label: "Not checked", cls: "" },
  working: { label: "Refreshing…", cls: "info" },
  disabled: { label: "Disabled", cls: "err" },
  errored: { label: "Error", cls: "err" },
  expired: { label: "Expired", cls: "warn" },
  gone: { label: "Gone", cls: "err" },
};

export function AccountsPage() {
  const accounts = useAccounts();
  const system = useQuery(() => materialmc.system.info(), []);
  const { showError } = useToasts();
  const [login, setLogin] = useState(false);
  const [offline, setOffline] = useState(false);
  const [removing, setRemoving] = useState<Account>();

  const run = (p: Promise<unknown>) => p.catch((e: unknown) => showError(e));

  return (
    <div className="page">
      <div className="page-header">
        <h1>Accounts</h1>
        <div className="actions">
          <button className="btn" onClick={() => setOffline(true)}>
            Add offline account
          </button>
          <button className="btn primary" disabled={system.data ? !system.data.capabilities.msa : false} onClick={() => setLogin(true)}>
            Add Microsoft account
          </button>
        </div>
      </div>

      {accounts.error ? (
        <ErrorBanner error={accounts.error} />
      ) : !accounts.loaded ? (
        <Spinner />
      ) : accounts.data.length === 0 ? (
        <Empty>No accounts yet. Add a Microsoft account that owns Minecraft to play online.</Empty>
      ) : (
        <div className="card flush">
          {accounts.data.map((a) => {
            const chip = STATE_CHIP[a.state];
            return (
              <div key={a.id} className="list-row" style={{ padding: 12 }}>
                <img src={a.faceUrl} alt="" width={40} height={40} style={{ borderRadius: 8, imageRendering: "pixelated" }} />
                <div className="grow" style={{ minWidth: 0 }}>
                  <div className="row">
                    <strong>{a.profileName || "(no profile)"}</strong>
                    {a.isDefault && <span className="chip ok">Default</span>}
                    <span className="chip">{a.type === "msa" ? "Microsoft" : "Offline"}</span>
                    <span className={`chip ${chip.cls}`}>{chip.label}</span>
                    {a.type === "msa" && !a.ownsMinecraft && <span className="chip warn">Does not own Minecraft</span>}
                  </div>
                  {a.lastError && <div className="small" style={{ color: "var(--danger)" }}>{a.lastError}</div>}
                </div>
                {!a.isDefault && (
                  <button className="btn small" onClick={() => void run(materialmc.accounts.setDefault(a.id))}>
                    Use as default
                  </button>
                )}
                <MenuButton
                  items={[
                    ...(a.type === "msa" ? [{ label: "Refresh", onSelect: () => void run(materialmc.accounts.refresh(a.id)) }] : []),
                    ...(a.isDefault ? [{ label: "Unset default", onSelect: () => void run(materialmc.accounts.setDefault(null)) }] : []),
                    { label: "Remove…", danger: true, separatorBefore: true, onSelect: () => setRemoving(a) },
                  ]}
                />
              </div>
            );
          })}
        </div>
      )}

      {login && <MsaLoginDialog onClose={() => setLogin(false)} />}
      {offline && <OfflineAccountDialog onClose={() => setOffline(false)} />}
      {removing && (
        <ConfirmDialog
          title={`Remove ${removing.profileName || "account"}?`}
          danger
          confirmLabel="Remove"
          message="The launcher forgets this account and its tokens. You can sign in again later."
          onClose={() => setRemoving(undefined)}
          onConfirm={() => run(materialmc.accounts.logout(removing.id)).then(() => undefined)}
        />
      )}
    </div>
  );
}

/**
 * Drives the backend MSA flow: the backend opens nothing by itself; it reports the
 * verification URL and device code through `account.login.*` events.
 */
function MsaLoginDialog({ onClose }: { onClose(): void }) {
  // A ref, not state: events can arrive before React re-renders with the new id.
  const flowId = useRef<string>(undefined);
  const [status, setStatus] = useState("Starting…");
  const [prompt, setPrompt] = useState<LoginPromptEvent>();
  const [error, setError] = useState<string>();
  const [started, setStarted] = useState(false);
  const { notify, showError } = useToasts();

  const start = async (useDeviceCode: boolean) => {
    setStarted(true);
    setError(undefined);
    setPrompt(undefined);
    try {
      const handle = await materialmc.accounts.login(useDeviceCode);
      flowId.current = handle.flowId;
    } catch (e) {
      setError(describeError(e).message);
    }
  };

  useEvent("account.login.status", (e) => e.flowId === flowId.current && setStatus(e.status));
  useEvent("account.login.prompt", (e) => e.flowId === flowId.current && setPrompt(e));
  useEvent("account.login.failed", (e) => e.flowId === flowId.current && setError(e.error.message));
  useEvent("account.login.finished", (e) => {
    if (e.flowId !== flowId.current) return;
    notify("Account added", "success");
    onClose();
  });

  const cancel = () => {
    if (flowId.current) materialmc.accounts.cancelLogin(flowId.current).catch((e: unknown) => showError(e));
    onClose();
  };

  return (
    <Dialog
      title="Sign in with Microsoft"
      onClose={cancel}
      footer={
        <button className="btn" onClick={cancel}>
          {error ? "Close" : "Cancel"}
        </button>
      }
    >
      {!started ? (
        <div className="stack">
          <button className="btn primary" onClick={() => void start(false)}>
            Sign in with the browser
          </button>
          <button className="btn" onClick={() => void start(true)}>
            Use a code on another device
          </button>
        </div>
      ) : error ? (
        <div className="banner error">{error}</div>
      ) : prompt ? (
        <div className="stack">
          {prompt.code ? (
            <>
              <div>
                Open <a href="#" onClick={(e) => (e.preventDefault(), void materialmc.system.openUrl(prompt.url))}>{prompt.url}</a> and
                enter this code:
              </div>
              <div className="code-box">{prompt.code}</div>
              <div className="row">
                <button className="btn" onClick={() => void materialmc.system.copyText(prompt.code ?? "")}>
                  Copy code
                </button>
                <button className="btn primary" onClick={() => void materialmc.system.openUrl(prompt.url)}>
                  Open page ↗
                </button>
              </div>
            </>
          ) : (
            <>
              <div>Continue signing in in your browser.</div>
              <button className="btn primary" onClick={() => void materialmc.system.openUrl(prompt.url)}>
                Open sign-in page ↗
              </button>
            </>
          )}
          <div className="small muted">{status}</div>
        </div>
      ) : (
        <Spinner label={status} />
      )}
    </Dialog>
  );
}

function OfflineAccountDialog({ onClose }: { onClose(): void }) {
  const [name, setName] = useState("");
  const { showError, notify } = useToasts();
  const valid = /^[A-Za-z0-9_]{1,16}$/.test(name);
  return (
    <Dialog
      title="Add offline account"
      onClose={onClose}
      footer={
        <>
          <button className="btn" onClick={onClose}>
            Cancel
          </button>
          <button
            className="btn primary"
            disabled={!valid}
            onClick={() =>
              materialmc.accounts.addOffline(name).then(
                () => {
                  notify("Offline account added", "success");
                  onClose();
                },
                (e: unknown) => showError(e),
              )
            }
          >
            Add
          </button>
        </>
      }
    >
      <p className="muted" style={{ margin: 0 }}>
        Offline accounts can only join offline-mode servers and single-player worlds.
      </p>
      <label className="field">
        <span>Player name</span>
        <input className="input" value={name} maxLength={16} onChange={(e) => setName(e.target.value)} autoFocus />
      </label>
    </Dialog>
  );
}
