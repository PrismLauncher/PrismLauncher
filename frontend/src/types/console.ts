/** Mirrors `MessageLevel` in launcher/MessageLevel.h. */
export type LogLevel =
  | "unknown"
  | "stdout"
  | "stderr"
  | "launcher"
  | "trace"
  | "debug"
  | "info"
  | "message"
  | "warning"
  | "error"
  | "fatal";

export interface LogLine {
  /** Monotonic line number within the current launch (survives LogModel ring-buffer wrap). */
  n: number;
  level: LogLevel;
  text: string;
}

export interface ConsoleLogParams {
  instanceId: string;
  /** Return lines with `n >= from`. */
  from?: number;
  /** Max lines returned (backend caps at 5000). */
  limit?: number;
}

export interface ConsoleLogChunk {
  instanceId: string;
  lines: LogLine[];
  /** Line number the next incremental fetch should start from. */
  next: number;
  /** False when the instance has never been launched in this session. */
  available: boolean;
}
