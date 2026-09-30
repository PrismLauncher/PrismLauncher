import type { ApiErrorPayload } from "./common";
import type { LogLine } from "./console";
import type { SettingsChangedEvent } from "./settings";
import type { TaskInfo } from "./tasks";

export interface InstanceEvent {
  instanceId: string;
}

export interface InstanceStoppedEvent extends InstanceEvent {
  /** False when the game exited with a non-zero status or crashed. */
  success: boolean;
  reason: string;
}

export interface InstanceLaunchFailedEvent extends InstanceEvent {
  error: ApiErrorPayload;
}

export interface MinecraftLogEvent extends InstanceEvent {
  /** Lines are batched (at most every ~50 ms) to keep IPC traffic low. */
  lines: LogLine[];
}

export interface LoginStatusEvent {
  flowId: string;
  status: string;
}

export interface LoginPromptEvent {
  flowId: string;
  /** Verification URL the user has to open. */
  url: string;
  /** Device code to type in, or `null` for the browser (auth-code) flow. */
  code: string | null;
  /** Seconds until the device code expires. */
  expiresIn: number | null;
}

export interface LoginFinishedEvent {
  flowId: string;
  accountId: string;
}

export interface LoginFailedEvent {
  flowId: string;
  error: ApiErrorPayload;
}

/**
 * Every event the backend can push, keyed by name.
 * Emitted from `launcher/api/*` via `ApiRouter::emitEvent`.
 */
export interface MaterialMCEventMap {
  "instances.changed": Record<string, never>;
  "instance.launching": InstanceEvent;
  "instance.started": InstanceEvent;
  "instance.stopped": InstanceStoppedEvent;
  "instance.launchFailed": InstanceLaunchFailedEvent;
  "instance.consoleRequested": InstanceEvent;
  "minecraft.log": MinecraftLogEvent;
  "minecraft.crashed": InstanceStoppedEvent;

  "download.started": TaskInfo;
  "download.progress": TaskInfo;
  "download.finished": TaskInfo;
  "download.failed": TaskInfo;

  "account.changed": Record<string, never>;
  "account.login.status": LoginStatusEvent;
  "account.login.prompt": LoginPromptEvent;
  "account.login.finished": LoginFinishedEvent;
  "account.login.failed": LoginFailedEvent;

  "settings.changed": SettingsChangedEvent;
  "resources.changed": { instanceId: string; kind: string };
  "worlds.changed": InstanceEvent;
}

export type MaterialMCEventName = keyof MaterialMCEventMap;
