import type { Timestamp } from "./common";
import type { LoaderKind } from "./instances";

/** Version types as published by the meta server (`release`, `snapshot`, `old_beta`, ...). */
export type MinecraftVersionType = string;

export interface MinecraftVersion {
  id: string;
  type: MinecraftVersionType;
  releaseTime: Timestamp | null;
  recommended: boolean;
}

export interface LoaderVersion {
  version: string;
  type: string;
  releaseTime: Timestamp | null;
  recommended: boolean;
}

export interface LoaderVersionsParams {
  loader: LoaderKind;
  minecraftVersion: string;
  forceReload?: boolean;
}

export interface JavaInstallation {
  path: string;
  version: string;
  architecture: string;
  recommended: boolean;
  isManaged: boolean;
}
