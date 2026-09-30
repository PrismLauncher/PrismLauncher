/**
 * Small event-driven stores for data that many screens share.
 *
 * Each store fetches once, then updates only in response to backend events;
 * there is no polling.
 */
import { useSyncExternalStore } from "react";
import { materialmc, on } from "../api/client";
import type { Account } from "../types/accounts";
import type { Instance } from "../types/instances";
import type { TaskInfo } from "../types/tasks";

interface Snapshot<T> {
  data: T;
  loaded: boolean;
  error: unknown;
}

class Store<T> {
  private snapshot: Snapshot<T>;
  private readonly listeners = new Set<() => void>();
  private started = false;
  private inflight: Promise<void> | null = null;
  private dirty = false;

  constructor(
    initial: T,
    private readonly fetcher: () => Promise<T>,
    private readonly subscribeEvents: (store: Store<T>) => void,
  ) {
    this.snapshot = { data: initial, loaded: false, error: null };
  }

  subscribe = (listener: () => void): (() => void) => {
    this.listeners.add(listener);
    if (!this.started) {
      this.started = true;
      this.subscribeEvents(this);
      void this.refresh();
    }
    return () => this.listeners.delete(listener);
  };

  getSnapshot = (): Snapshot<T> => this.snapshot;

  set(update: (data: T) => T): void {
    this.snapshot = { ...this.snapshot, data: update(this.snapshot.data) };
    this.emit();
  }

  /** Refetches; concurrent requests collapse into one follow-up fetch. */
  refresh(): Promise<void> {
    if (this.inflight) {
      this.dirty = true;
      return this.inflight;
    }
    this.inflight = this.fetcher()
      .then(
        (data) => {
          this.snapshot = { data, loaded: true, error: null };
        },
        (error: unknown) => {
          this.snapshot = { ...this.snapshot, loaded: true, error };
        },
      )
      .finally(() => {
        this.inflight = null;
        this.emit();
        if (this.dirty) {
          this.dirty = false;
          void this.refresh();
        }
      });
    return this.inflight;
  }

  private emit(): void {
    for (const l of this.listeners) l();
  }
}

export const instancesStore = new Store<Instance[]>([], materialmc.instances.list, (store) => {
  const refresh = () => void store.refresh();
  on("instances.changed", refresh);
  const setState = (id: string, state: Instance["state"]) =>
    store.set((list) => list.map((i) => (i.id === id ? { ...i, state } : i)));
  on("instance.launching", (e) => setState(e.instanceId, "launching"));
  on("instance.started", (e) => setState(e.instanceId, "running"));
  // Stopping updates play time and "last launched", so refetch.
  on("instance.stopped", refresh);
  on("instance.launchFailed", refresh);
  on("resources.changed", refresh);
});

export const accountsStore = new Store<Account[]>([], materialmc.accounts.list, (store) => {
  on("account.changed", () => void store.refresh());
});

const MAX_FINISHED_TASKS = 100;

export const tasksStore = new Store<TaskInfo[]>([], materialmc.tasks.list, (store) => {
  const upsert = (task: TaskInfo) =>
    store.set((list) => {
      const index = list.findIndex((t) => t.id === task.id);
      if (index === -1) return [task, ...list].slice(0, MAX_FINISHED_TASKS);
      const copy = list.slice();
      copy[index] = task;
      return copy;
    });
  on("download.started", upsert);
  on("download.progress", upsert);
  on("download.finished", upsert);
  on("download.failed", upsert);
});

function useStore<T>(store: Store<T>): Snapshot<T> {
  return useSyncExternalStore(store.subscribe, store.getSnapshot);
}

export const useInstances = () => useStore(instancesStore);
export const useAccounts = () => useStore(accountsStore);
export const useTasks = () => useStore(tasksStore);

export function refreshInstances(): Promise<void> {
  return instancesStore.refresh();
}
export function refreshTasks(): Promise<void> {
  return tasksStore.refresh();
}
