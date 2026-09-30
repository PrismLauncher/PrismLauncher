/**
 * Typed public API of the backend: `window.materialmc`.
 *
 * Only named, purpose-specific operations exist here. There is deliberately no
 * generic "execute" / "call any C++ function" entry point.
 */
import { bridge } from "./bridge";
import type { MaterialMCEventMap, MaterialMCEventName } from "../types/events";
import type { MaterialMCMethods, MethodName, MethodParams, MethodResult } from "../types/methods";

function call<M extends MethodName>(method: M, params: MethodParams<M>): Promise<MethodResult<M>> {
  return bridge.call(method, params) as Promise<MethodResult<M>>;
}

/** Builds `(params) => call(method, params)`, or `() => call(method, {})` for parameterless methods. */
function method<M extends MethodName>(name: M) {
  return (params: MethodParams<M>) => call(name, params);
}
function noArgs<M extends MethodName>(name: M) {
  return () => call(name, {} as MethodParams<M>);
}

export function on<E extends MaterialMCEventName>(
  name: E,
  callback: (payload: MaterialMCEventMap[E]) => void,
): () => void {
  return bridge.on(name, callback as (payload: unknown) => void);
}

export const materialmc = {
  get connected(): boolean {
    return bridge.available;
  },
  on,

  system: {
    info: noArgs("system.info"),
    methods: noArgs("system.methods"),
    openFolder: method("system.openFolder"),
    openUrl: (url: string) => call("system.openUrl", { url }),
    copyText: (text: string) => call("system.copyText", { text }),
    saveText: method("system.saveText"),
    icons: noArgs("system.icons"),
  },

  instances: {
    list: noArgs("instances.list"),
    groups: noArgs("instances.groups"),
    get: (id: string) => call("instances.get", { id }),
    create: method("instances.create"),
    copy: method("instances.copy"),
    remove: (id: string) => call("instances.remove", { id }),
    rename: (id: string, name: string) => call("instances.rename", { id, name }),
    setGroup: (id: string, group: string | null) => call("instances.setGroup", { id, group }),
    setIcon: (id: string, iconKey: string) => call("instances.setIcon", { id, iconKey }),
    setNotes: (id: string, notes: string) => call("instances.setNotes", { id, notes }),
    getSettings: (id: string) => call("instances.getSettings", { id }),
    setSettings: method("instances.setSettings"),
    launch: method("instances.launch"),
    kill: (id: string) => call("instances.kill", { id }),
  },

  resources: {
    list: method("resources.list"),
    setEnabled: method("resources.setEnabled"),
    remove: method("resources.remove"),
    importFiles: method("resources.importFiles"),
  },
  worlds: {
    list: (instanceId: string) => call("worlds.list", { instanceId }),
    remove: method("worlds.remove"),
    rename: method("worlds.rename"),
  },
  screenshots: {
    list: (instanceId: string) => call("screenshots.list", { instanceId }),
    remove: method("screenshots.remove"),
  },
  logs: {
    list: (instanceId: string) => call("logs.list", { instanceId }),
    read: method("logs.read"),
  },

  accounts: {
    list: noArgs("accounts.list"),
    login: (useDeviceCode = false) => call("accounts.loginMsa", { useDeviceCode }),
    cancelLogin: (flowId: string) => call("accounts.cancelLogin", { flowId }),
    addOffline: (name: string) => call("accounts.addOffline", { name }),
    logout: (id: string) => call("accounts.remove", { id }),
    setDefault: (id: string | null) => call("accounts.setDefault", { id }),
    refresh: (id: string) => call("accounts.refresh", { id }),
  },

  versions: {
    minecraft: method("versions.minecraft"),
    loaders: method("versions.loaders"),
  },
  java: {
    list: method("java.list"),
  },

  mods: {
    search: method("mods.search"),
    versions: method("mods.versions"),
    install: method("mods.install"),
  },

  settings: {
    get: noArgs("settings.get"),
    set: method("settings.set"),
  },

  tasks: {
    list: noArgs("tasks.list"),
    cancel: (taskId: string) => call("tasks.cancel", { taskId }),
    clearFinished: noArgs("tasks.clearFinished"),
  },

  console: {
    get: method("console.get"),
    launcherLog: method("console.launcherLog"),
  },
} as const;

export type MaterialMC = typeof materialmc;

declare global {
  interface Window {
    materialmc: MaterialMC;
  }
}

// Exposed for debugging from the WebView inspector; the UI itself imports the module.
window.materialmc = materialmc;

/** Every method name of the contract, used for the dev-mode drift check. */
const CONTRACT: Record<MethodName, true> = {
  "system.info": true, "system.methods": true, "system.openFolder": true, "system.openUrl": true,
  "system.copyText": true, "system.saveText": true, "system.icons": true,
  "instances.list": true, "instances.groups": true, "instances.get": true, "instances.create": true,
  "instances.copy": true, "instances.remove": true, "instances.rename": true, "instances.setGroup": true,
  "instances.setIcon": true, "instances.setNotes": true, "instances.getSettings": true,
  "instances.setSettings": true, "instances.launch": true, "instances.kill": true,
  "resources.list": true, "resources.setEnabled": true, "resources.remove": true, "resources.importFiles": true,
  "worlds.list": true, "worlds.remove": true, "worlds.rename": true,
  "screenshots.list": true, "screenshots.remove": true, "logs.list": true, "logs.read": true,
  "accounts.list": true, "accounts.loginMsa": true, "accounts.cancelLogin": true, "accounts.addOffline": true,
  "accounts.remove": true, "accounts.setDefault": true, "accounts.refresh": true,
  "versions.minecraft": true, "versions.loaders": true, "java.list": true,
  "mods.search": true, "mods.versions": true, "mods.install": true,
  "settings.get": true, "settings.set": true,
  "tasks.list": true, "tasks.cancel": true, "tasks.clearFinished": true,
  "console.get": true, "console.launcherLog": true,
} satisfies Record<keyof MaterialMCMethods, true>;

/** Logs methods that exist on only one side of the contract. */
export async function checkContract(): Promise<void> {
  const remote = new Set(await materialmc.system.methods());
  const local = Object.keys(CONTRACT);
  const missing = local.filter((m) => !remote.has(m));
  const extra = [...remote].filter((m) => !(m in CONTRACT));
  if (missing.length || extra.length) {
    console.warn("[materialmc] API contract drift", { missingInBackend: missing, unknownToFrontend: extra });
  }
}
