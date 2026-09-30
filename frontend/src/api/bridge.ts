/**
 * Low-level transport between the React UI and the C++ host.
 *
 * Wire protocol (JSON strings in both directions):
 *   UI  -> host : { kind: "call", id, method, params }
 *   host -> UI  : { kind: "result", id, ok: true, result }
 *               | { kind: "result", id, ok: false, error: { code, message, details? } }
 *               | { kind: "event", name, payload }
 *
 * The host injects `window.__materialmcNative.post(string)` before any page
 * script runs (see `launcher/webview/WebView.cpp`, `bootstrapScript()`), and
 * delivers messages by calling `window.__materialmcReceive(string)`. The
 * platform-specific plumbing (WebKit message handlers, WebView2 postMessage,
 * WKScriptMessageHandler) is hidden behind those two functions.
 */
import { MaterialMCError, type ApiErrorPayload } from "../types/common";

interface NativeChannel {
  post(message: string): void;
}

declare global {
  interface Window {
    __materialmcNative?: NativeChannel;
    __materialmcReceive?: (message: string) => void;
  }
}

type ResultMessage =
  | { kind: "result"; id: number; ok: true; result: unknown }
  | { kind: "result"; id: number; ok: false; error: ApiErrorPayload };

type EventMessage = { kind: "event"; name: string; payload: unknown };

type IncomingMessage = ResultMessage | EventMessage;

interface Pending {
  resolve(value: unknown): void;
  reject(error: MaterialMCError): void;
}

type EventListener = (payload: unknown) => void;

function isObject(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null;
}

function parseIncoming(raw: string): IncomingMessage | null {
  let value: unknown;
  try {
    value = JSON.parse(raw);
  } catch {
    return null;
  }
  if (!isObject(value)) return null;
  if (value.kind === "result" && typeof value.id === "number" && typeof value.ok === "boolean") {
    return value as IncomingMessage;
  }
  if (value.kind === "event" && typeof value.name === "string") {
    return value as IncomingMessage;
  }
  return null;
}

class Bridge {
  private nextId = 1;
  private readonly pending = new Map<number, Pending>();
  private readonly listeners = new Map<string, Set<EventListener>>();

  constructor() {
    window.__materialmcReceive = (raw: string) => this.receive(raw);
  }

  get available(): boolean {
    return typeof window.__materialmcNative?.post === "function";
  }

  call(method: string, params: unknown): Promise<unknown> {
    const native = window.__materialmcNative;
    if (!native) {
      return Promise.reject(
        new MaterialMCError({
          code: "BRIDGE_UNAVAILABLE",
          message: "The MaterialMC backend is not connected. Open this UI from the MaterialMC application.",
        }),
      );
    }
    const id = this.nextId++;
    return new Promise((resolve, reject) => {
      this.pending.set(id, { resolve, reject });
      try {
        native.post(JSON.stringify({ kind: "call", id, method, params: params ?? {} }));
      } catch (e) {
        this.pending.delete(id);
        reject(new MaterialMCError({ code: "BRIDGE_UNAVAILABLE", message: String(e) }));
      }
    });
  }

  on(name: string, listener: EventListener): () => void {
    let set = this.listeners.get(name);
    if (!set) {
      set = new Set();
      this.listeners.set(name, set);
    }
    set.add(listener);
    return () => {
      set.delete(listener);
    };
  }

  private receive(raw: string): void {
    const message = parseIncoming(raw);
    if (!message) {
      console.warn("[materialmc] dropped malformed message from host");
      return;
    }
    if (message.kind === "event") {
      const set = this.listeners.get(message.name);
      if (!set) return;
      for (const listener of [...set]) {
        try {
          listener(message.payload);
        } catch (e) {
          console.error(`[materialmc] listener for ${message.name} threw`, e);
        }
      }
      return;
    }
    const pending = this.pending.get(message.id);
    if (!pending) return;
    this.pending.delete(message.id);
    if (message.ok) {
      pending.resolve(message.result);
    } else {
      pending.reject(new MaterialMCError(message.error));
    }
  }
}

export const bridge = new Bridge();
