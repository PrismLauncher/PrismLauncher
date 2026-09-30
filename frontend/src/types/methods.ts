/**
 * The complete RPC surface of the backend: method name -> [params, result].
 *
 * This table is the contract between `frontend/src/api` and
 * `launcher/api/*Api.cpp`. The backend exposes the list of registered names via
 * `system.methods`; in dev mode the frontend compares both lists at start-up
 * and logs any drift (see `api/client.ts`).
 */
import type { TaskHandle } from "./common";
import type { Account, LoginFlowHandle } from "./accounts";
import type { ConsoleLogChunk, ConsoleLogParams } from "./console";
import type {
  CopyInstanceParams,
  CreateInstanceParams,
  Instance,
  InstanceDetails,
  InstanceId,
  InstanceSettings,
  LaunchParams,
} from "./instances";
import type {
  ModInstallParams,
  ModSearchParams,
  ModSearchResult,
  RemoteVersion,
  RemoteVersionsParams,
} from "./mods";
import type {
  GameLogContent,
  GameLogFile,
  Resource,
  ResourceListParams,
  ResourceSelection,
  Screenshot,
  World,
} from "./resources";
import type { LauncherSettings } from "./settings";
import type { IconInfo, OpenFolderParams, SaveTextParams, SystemInfo } from "./system";
import type { TaskInfo } from "./tasks";
import type { JavaInstallation, LoaderVersion, LoaderVersionsParams, MinecraftVersion } from "./versions";

type Id = { id: InstanceId };
type Empty = Record<string, never>;
type Ok = { ok: true };

export interface MaterialMCMethods {
  // system
  "system.info": [Empty, SystemInfo];
  "system.methods": [Empty, string[]];
  "system.openFolder": [OpenFolderParams, Ok];
  "system.openUrl": [{ url: string }, Ok];
  "system.copyText": [{ text: string }, Ok];
  "system.saveText": [SaveTextParams, { saved: boolean }];
  "system.icons": [Empty, IconInfo[]];

  // instances
  "instances.list": [Empty, Instance[]];
  "instances.groups": [Empty, string[]];
  "instances.get": [Id, InstanceDetails];
  "instances.create": [CreateInstanceParams, TaskHandle];
  "instances.copy": [CopyInstanceParams, TaskHandle];
  "instances.remove": [Id, Ok];
  "instances.rename": [Id & { name: string }, Ok];
  "instances.setGroup": [Id & { group: string | null }, Ok];
  "instances.setIcon": [Id & { iconKey: string }, Ok];
  "instances.setNotes": [Id & { notes: string }, Ok];
  "instances.getSettings": [Id, InstanceSettings];
  "instances.setSettings": [Id & { settings: Partial<InstanceSettings> }, InstanceSettings];
  "instances.launch": [LaunchParams, Ok];
  "instances.kill": [Id, Ok];

  // instance content
  "resources.list": [ResourceListParams, Resource[]];
  "resources.setEnabled": [ResourceSelection & { enabled: boolean }, Ok];
  "resources.remove": [ResourceSelection, Ok];
  "resources.importFiles": [ResourceListParams, { imported: number }];
  "worlds.list": [{ instanceId: InstanceId }, World[]];
  "worlds.remove": [{ instanceId: InstanceId; worldId: string }, TaskHandle];
  "worlds.rename": [{ instanceId: InstanceId; worldId: string; name: string }, Ok];
  "screenshots.list": [{ instanceId: InstanceId }, Screenshot[]];
  "screenshots.remove": [{ instanceId: InstanceId; names: string[] }, Ok];
  "logs.list": [{ instanceId: InstanceId }, GameLogFile[]];
  "logs.read": [{ instanceId: InstanceId; name: string }, GameLogContent];

  // accounts
  "accounts.list": [Empty, Account[]];
  "accounts.loginMsa": [{ useDeviceCode?: boolean }, LoginFlowHandle];
  "accounts.cancelLogin": [LoginFlowHandle, Ok];
  "accounts.addOffline": [{ name: string }, Account];
  "accounts.remove": [{ id: string }, Ok];
  "accounts.setDefault": [{ id: string | null }, Ok];
  "accounts.refresh": [{ id: string }, Ok];

  // versions / java
  "versions.minecraft": [{ forceReload?: boolean }, MinecraftVersion[]];
  "versions.loaders": [LoaderVersionsParams, LoaderVersion[]];
  "java.list": [{ forceReload?: boolean }, JavaInstallation[]];

  // mod platforms
  "mods.search": [ModSearchParams, ModSearchResult];
  "mods.versions": [RemoteVersionsParams, RemoteVersion[]];
  "mods.install": [ModInstallParams, TaskHandle];

  // settings
  "settings.get": [Empty, LauncherSettings];
  "settings.set": [{ values: Partial<LauncherSettings> }, LauncherSettings];

  // tasks / downloads
  "tasks.list": [Empty, TaskInfo[]];
  "tasks.cancel": [{ taskId: string }, Ok];
  "tasks.clearFinished": [Empty, Ok];

  // console
  "console.get": [ConsoleLogParams, ConsoleLogChunk];
  "console.launcherLog": [{ from?: number; limit?: number }, Omit<ConsoleLogChunk, "instanceId">];
}

export type MethodName = keyof MaterialMCMethods;
export type MethodParams<M extends MethodName> = MaterialMCMethods[M][0];
export type MethodResult<M extends MethodName> = MaterialMCMethods[M][1];
