import type { Timestamp } from "./common";

export type InstanceId = string;

/** Mod loader component uids from the Prism meta server, mapped to short names. */
export type LoaderKind = "fabric" | "quilt" | "forge" | "neoforge" | "liteloader";

export interface InstanceLoader {
  kind: LoaderKind;
  version: string;
}

/**
 * Runtime state of an instance.
 * - `launching`: the launch pipeline (auth, update, pre-launch) is running.
 * - `running`: the Minecraft process is alive.
 */
export type InstanceRunState = "stopped" | "launching" | "running";

/** Serialized by `InstanceApi::serializeInstance` (launcher/api/InstanceApi.cpp). */
export interface Instance {
  id: InstanceId;
  name: string;
  group: string | null;
  /** `materialmc://app/_icon/<key>` URL served by the host. */
  iconUrl: string;
  iconKey: string;
  minecraftVersion: string | null;
  loader: InstanceLoader | null;
  state: InstanceRunState;
  /** Number of files in the loader mods folder (cheap directory count, not a parsed list). */
  modCount: number;
  lastLaunch: Timestamp | null;
  /** Seconds. */
  totalPlayTime: number;
  /** Seconds. */
  lastPlayTime: number;
  canLaunch: boolean;
  hasCrashed: boolean;
  hasVersionBroken: boolean;
  managedPack: { type: string; name: string; version: string } | null;
}

export interface InstanceDetails extends Instance {
  notes: string;
  /** Absolute path, informational only (the UI never passes paths back). */
  instanceRoot: string;
  gameRoot: string;
  components: InstanceComponent[];
}

export interface InstanceComponent {
  uid: string;
  name: string;
  version: string;
  enabled: boolean;
  important: boolean;
}

/** Per-instance overrides that the web UI may edit. Mirrors keys in MinecraftInstance settings. */
export interface InstanceSettings {
  overrideMemory: boolean;
  minMemAlloc: number;
  maxMemAlloc: number;
  overrideJavaLocation: boolean;
  javaPath: string;
  overrideJavaArgs: boolean;
  jvmArgs: string;
  overrideWindow: boolean;
  launchMaximized: boolean;
  minecraftWinWidth: number;
  minecraftWinHeight: number;
  useAccountForInstance: boolean;
  instanceAccountId: string;
  joinServerOnLaunch: boolean;
  joinServerOnLaunchAddress: string;
}

export interface CreateInstanceParams {
  name: string;
  group?: string | null;
  iconKey?: string;
  minecraftVersion: string;
  loader?: InstanceLoader | null;
}

export interface CopyInstanceParams {
  id: InstanceId;
  name: string;
  group?: string | null;
  copySaves: boolean;
  keepPlaytime: boolean;
}

export type LaunchMode = "normal" | "offline" | "demo";

export interface LaunchParams {
  id: InstanceId;
  mode?: LaunchMode;
  /** Player name for offline / demo launches. */
  offlineName?: string;
  /** `Account.id` to use instead of the instance / global default. */
  accountId?: string;
  /** `host[:port]` to join on start. */
  server?: string;
  /** World folder name to join on start. */
  world?: string;
}
