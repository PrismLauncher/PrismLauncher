import type { ApiErrorPayload, Timestamp } from "./common";

/** What kind of backend work a tracked task represents. */
export type TaskKind =
  | "instance.create"
  | "instance.copy"
  | "mod.install"
  | "world.delete"
  | "account.login"
  | "launch.update"
  | "java.detect"
  | "versions.load"
  | "other";

export type TaskState = "running" | "succeeded" | "failed" | "aborted";

/** Serialized by `TaskTracker::serialize` (launcher/api/TaskTracker.cpp). */
export interface TaskInfo {
  id: string;
  kind: TaskKind;
  title: string;
  status: string;
  details: string;
  state: TaskState;
  /** Current/total progress. `total <= 0` means indeterminate. */
  current: number;
  total: number;
  canAbort: boolean;
  startedAt: Timestamp;
  finishedAt: Timestamp | null;
  error: ApiErrorPayload | null;
  /** Related instance, when any. */
  instanceId: string | null;
}
