import type { Timestamp } from "./common";
import type { InstanceId } from "./instances";

/** Folder-backed resource lists of a Minecraft instance. */
export type ResourceKind = "mods" | "resourcepacks" | "shaderpacks" | "texturepacks";

/** Serialized by `ResourceApi::serializeResource`. */
export interface Resource {
  /** `Resource::internalId()` – the file name including `.disabled`. */
  id: string;
  name: string;
  fileName: string;
  version: string;
  enabled: boolean;
  /** Mod platform the file was installed from (`Modrinth`, `CurseForge`) or `null`. */
  provider: string | null;
  sizeBytes: number;
  modified: Timestamp | null;
  homepage: string;
  issues: string[];
}

export interface ResourceListParams {
  instanceId: InstanceId;
  kind: ResourceKind;
}

export interface ResourceSelection extends ResourceListParams {
  ids: string[];
}

/** Serialized by `ResourceApi::serializeWorld`. */
export interface World {
  /** Folder name inside `saves/`. */
  id: string;
  name: string;
  gameType: string;
  lastPlayed: Timestamp | null;
  sizeBytes: number;
  /** Decimal string, int64 does not fit in a JS number. */
  seed: string;
  iconUrl: string | null;
  isValid: boolean;
}

export interface Screenshot {
  name: string;
  url: string;
  modified: Timestamp;
  sizeBytes: number;
}

export interface GameLogFile {
  name: string;
  sizeBytes: number;
  modified: Timestamp;
}

export interface GameLogContent {
  name: string;
  content: string;
  /** True when only the tail of a large file was returned. */
  truncated: boolean;
}
