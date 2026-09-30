import type { InstanceId } from "./instances";

export interface SystemInfo {
  name: string;
  displayName: string;
  version: string;
  gitCommit: string;
  buildPlatform: string;
  os: string;
  qtVersion: string;
  webViewEngine: string;
  isPortable: boolean;
  dataPath: string;
  devMode: boolean;
  capabilities: {
    msa: boolean;
    curseforge: boolean;
    gameMode: boolean;
    mangoHud: boolean;
  };
  urls: {
    bugTracker: string;
    wiki: string;
    discord: string;
    matrix: string;
    subreddit: string;
    translations: string;
  };
}

/**
 * Folders the UI may ask the backend to open in the system file manager.
 * The UI never supplies a path: it names a target and (for instance targets) an instance id.
 */
export type FolderTarget =
  | "data"
  | "instances"
  | "icons"
  | "logs"
  | "java"
  | "instance"
  | "instance.game"
  | "instance.mods"
  | "instance.resourcepacks"
  | "instance.shaderpacks"
  | "instance.texturepacks"
  | "instance.saves"
  | "instance.screenshots"
  | "instance.logs";

export interface OpenFolderParams {
  target: FolderTarget;
  instanceId?: InstanceId;
}

export interface SaveTextParams {
  suggestedName: string;
  content: string;
}

export interface IconInfo {
  key: string;
  name: string;
  url: string;
}
