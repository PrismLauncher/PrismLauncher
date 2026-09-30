/**
 * Shared primitives of the C++ <-> TypeScript contract.
 *
 * Every type in `src/types` mirrors a JSON shape produced by a serializer in
 * `launcher/api/*.cpp`. When changing one side, change the other.
 */

/** Milliseconds since the Unix epoch. `null` when the backend does not know the value. */
export type Timestamp = number;

/** Machine-readable error codes produced by `launcher/api/ApiError.h`. */
export type ApiErrorCode =
  | "INVALID_PARAMS"
  | "UNKNOWN_METHOD"
  | "NOT_FOUND"
  | "INSTANCE_NOT_FOUND"
  | "INSTANCE_RUNNING"
  | "INSTANCE_NOT_RUNNING"
  | "INSTANCE_LINKED"
  | "ACCOUNT_NOT_FOUND"
  | "NO_ACCOUNT"
  | "ACCOUNT_NEEDS_REAUTH"
  | "ACCOUNT_NEEDS_PROFILE"
  | "LAUNCH_FAILED"
  | "TASK_NOT_FOUND"
  | "TASK_FAILED"
  | "NETWORK_ERROR"
  | "UNSUPPORTED"
  | "PERMISSION_DENIED"
  | "CANCELLED"
  | "IO_ERROR"
  | "BRIDGE_UNAVAILABLE"
  | "INTERNAL_ERROR";

export interface ApiErrorPayload {
  code: ApiErrorCode;
  message: string;
  details?: unknown;
}

/** Error thrown by every `window.materialmc.*` call that the backend rejected. */
export class MaterialMCError extends Error {
  readonly code: ApiErrorCode;
  readonly details: unknown;

  constructor(payload: ApiErrorPayload) {
    super(payload.message);
    this.name = "MaterialMCError";
    this.code = payload.code;
    this.details = payload.details;
  }
}

export function isMaterialMCError(value: unknown): value is MaterialMCError {
  return value instanceof MaterialMCError;
}

/** Handle for a long-running backend operation, see `types/tasks.ts`. */
export interface TaskHandle {
  taskId: string;
}
