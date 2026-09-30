import type { InstanceId } from "./instances";
import type { ResourceKind } from "./resources";

export type ModProvider = "modrinth" | "curseforge";

export interface ModSearchParams {
  instanceId: InstanceId;
  kind: ResourceKind;
  provider: ModProvider;
  query: string;
  offset?: number;
  /** One of `ModSearchResult.sortingMethods[].id`. */
  sort?: string;
}

export interface RemoteProject {
  provider: ModProvider;
  id: string;
  slug: string;
  name: string;
  description: string;
  authors: string[];
  /** Remote https URL of the project icon (may be empty). */
  iconUrl: string;
  websiteUrl: string;
}

export interface SortingMethod {
  id: string;
  name: string;
}

export interface ModSearchResult {
  projects: RemoteProject[];
  offset: number;
  sortingMethods: SortingMethod[];
}

export interface RemoteVersionsParams {
  instanceId: InstanceId;
  kind: ResourceKind;
  provider: ModProvider;
  projectId: string;
}

export interface RemoteVersion {
  id: string;
  name: string;
  versionNumber: string;
  type: string;
  gameVersions: string[];
  loaders: string[];
  date: string;
  fileName: string;
  compatible: boolean;
}

export interface ModInstallParams extends RemoteVersionsParams {
  versionId: string;
}
