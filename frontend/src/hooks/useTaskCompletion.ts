import { on } from "../api/client";
import { MaterialMCError } from "../types/common";
import type { TaskInfo } from "../types/tasks";

/**
 * Resolves when the backend task finishes successfully, rejects when it fails or is aborted.
 * Used by screens that start a task and want to navigate once it is done.
 */
export function waitForTask(taskId: string): Promise<TaskInfo> {
  return new Promise((resolve, reject) => {
    const offs: Array<() => void> = [];
    const done = () => offs.forEach((off) => off());
    offs.push(
      on("download.finished", (task) => {
        if (task.id !== taskId) return;
        done();
        resolve(task);
      }),
      on("download.failed", (task) => {
        if (task.id !== taskId) return;
        done();
        reject(
          new MaterialMCError(
            task.error ?? { code: task.state === "aborted" ? "CANCELLED" : "TASK_FAILED", message: task.status },
          ),
        );
      }),
    );
  });
}
