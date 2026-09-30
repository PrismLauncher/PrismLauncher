# MaterialMC web UI: React + system WebView

MaterialMC's interface is a React + TypeScript application rendered by the operating system's WebView.
All launcher logic — instances, accounts, authentication, downloads, metadata, Java, the launch pipeline,
the game process — stays in the existing C++/Qt core. The UI talks to it only through a typed,
purpose-specific RPC API.

```text
                    MaterialMC process
                          │
          ┌───────────────┴────────────────┐
          │                                │
     C++ core (unchanged)            Web UI (frontend/)
  InstanceList, LaunchController,    React 19 + TypeScript
  AccountList/AuthFlow, Meta,        window.materialmc (typed)
  ResourceAPI, SettingsObject, ...        │
          │                                │
     launcher/api/*Api  ◄──── ApiRouter ◄──┤  JSON messages
          │                  (RPC+events)  │
          └──────── launcher/webview ──────┘
                   WebKitGTK / WebView2 / WKWebView
```

## Contents

1. [Status and migration phases](#status-and-migration-phases)
2. [Repository layout](#repository-layout)
3. [WebView layer](#webview-layer)
4. [IPC protocol](#ipc-protocol)
5. [C++ API](#c-api)
6. [React API](#react-api)
7. [Events](#events)
8. [Launching](#launching)
9. [Security model](#security-model)
10. [Building](#building)
11. [Development mode](#development-mode)
12. [Debugging](#debugging)
13. [Adding an API method](#adding-an-api-method)
14. [Known gaps](#known-gaps)

## Status and migration phases

| Phase | Content | State |
| --- | --- | --- |
| 1 | Qt GUI + new API + WebView | done |
| 2 | React replaces individual screens | done |
| 3 | React is the main GUI | done on Linux; Qt GUI remains as fallback (`--qt-gui`) |
| 4 | Remove obsolete Qt GUI classes | **not started** — see [Known gaps](#known-gaps) |

The web UI is the main window whenever it can start (Linux/BSD with WebKitGTK and the bundled
frontend). It falls back to the Qt Widgets GUI when:

* `--qt-gui` is passed or `MATERIALMC_QT_GUI=1` is set,
* the build has `Launcher_ENABLE_WEBUI=OFF`, or no WebView backend exists for the platform
  (Windows and macOS for now),
* the WebView cannot be created (no display, missing runtime), or the frontend bundle is missing.

Phase 4 is intentionally postponed: several flows still exist only in Qt (listed below), and much of
`launcher/ui` is also used by backend classes (`LaunchController`, `InstanceTask`, `JavaCommon`,
the updater). Qt itself is *not* a GUI-only dependency — the whole core is built on QtCore/QtNetwork
and the models are `QAbstractItemModel`s — so Qt stays.

## Repository layout

```text
frontend/                     React app (Vite, strict TypeScript)
├── src/types/                the C++ <-> TS contract: every method, parameter, result and event
├── src/api/bridge.ts         transport (window.__materialmcNative / window.__materialmcReceive)
├── src/api/client.ts         typed public API: window.materialmc
├── src/hooks/                event-driven stores (instances, accounts, tasks), useQuery, useEvent
├── src/components/           layout, dialogs, virtualized list, console, resource tables, ...
├── src/pages/                one file per screen
└── CMakeLists.txt            npm ci / npm run build, staging and installation

launcher/webview/             platform WebView abstraction
├── WebView.h / WebView.cpp   interface, security contract, shared helpers
├── LinuxWebView.cpp          WebKitGTK 4.1 backend
├── WindowsWebView.cpp        WebView2 backend (stub, TODO)
└── MacOSWebView.mm           WKWebView backend (stub, TODO)

launcher/api/                 RPC layer between the page and the core
├── ApiRouter                 method registry, dispatch, events
├── ApiError / ApiParams      structured errors, validated parameter access
├── TaskTracker               backend Tasks -> download.* events (replaces ProgressDialog)
├── InstanceApi               instances.*, launch lifecycle (implements LaunchInteraction)
├── ConsoleApi                console.*, minecraft.log stream
├── ResourceApi               resources.*, worlds.*, screenshots.*, logs.*
├── AccountApi                accounts.*, account.login.* events
├── VersionApi                versions.*, java.list
├── ModApi                    mods.* (Modrinth / CurseForge)
├── SettingsApi               settings.* (allow-list)
├── SystemApi                 system.*, tasks.*
├── SensitiveChange.h         native confirmation for command-line settings
└── WebUiHost                 owns window + router + modules, serves materialmc://app/

launcher/LaunchInteraction.h  lets LaunchController ask questions without Qt dialogs
```

## WebView layer

`webview::WebView` (launcher/webview/WebView.h) is a native top-level window hosting the system WebView.
The interface is deliberately small:

```cpp
static std::unique_ptr<WebView> create(const Options&, QString* error);
void load(const QUrl&);
void show(); void hide(); void present(); void setTitle(const QString&);
void postMessage(const QByteArray& json);          // host -> page
void setMessageHandler(MessageHandler);            // page -> host
void setResourceHandler(ResourceHandler);          // serves materialmc://app/<path>
void setExternalUrlHandler(ExternalUrlHandler);    // user clicked an http(s) link
void setCloseHandler(CloseHandler);
```

### Linux: WebKitGTK 4.1

* **Why WebKitGTK:** it is the system WebView on Linux (used by GNOME Web, Tauri, webview/webview), it is
  packaged by every distribution, and it supports custom URI schemes, user scripts and script message
  handlers natively. QtWebEngine was rejected: it bundles Chromium.
* **GTK 3 (`webkit2gtk-4.1`)** rather than GTK 4 (`webkitgtk-6.0`) because it is available on more
  distributions today; the backend only uses API that exists in both, so moving to 6.0 is mechanical.
* **Event loop:** Qt on Linux uses a GLib-based event dispatcher that iterates the default
  `GMainContext`, which is also what GTK and WebKit use. Both toolkits therefore run on the GUI thread
  without a second loop or thread. If Qt was built without GLib, `LinuxWebView.cpp` pumps the GLib
  context from a Qt timer.
* **Qt keyword macros:** GLib headers use `signals` as an identifier; `LinuxWebView.cpp` undefines
  `signals`/`slots`/`emit` around the GTK includes and is excluded from the Qt precompiled header.
* **Web process crashes** are logged and the page is reloaded.

### Windows (WebView2) and macOS (WKWebView)

`WindowsWebView.cpp` and `MacOSWebView.mm` contain the implementation plan as comments and currently
report "not implemented", so those platforms keep the Qt GUI. Everything above the `WebView` interface
(API layer, frontend, build integration) is platform independent. A backend must implement the
[security contract](#security-model) — the comments list the corresponding WebView2/WKWebView calls.

## IPC protocol

All messages are JSON strings.

```jsonc
// page -> host
{ "kind": "call", "id": 7, "method": "instances.launch", "params": { "id": "My Pack" } }

// host -> page: result
{ "kind": "result", "id": 7, "ok": true, "result": { "ok": true } }
{ "kind": "result", "id": 7, "ok": false,
  "error": { "code": "INSTANCE_NOT_FOUND", "message": "Instance 'x' does not exist" } }

// host -> page: event
{ "kind": "event", "name": "download.progress", "payload": { "id": "t3", "current": 10, "total": 40, ... } }
```

* **Page -> host:** a bootstrap script injected before any page script defines
  `window.__materialmcNative.post(string)`. On WebKitGTK it forwards to the `materialmc` script message
  handler.
* **Host -> page:** the host evaluates exactly one kind of script,
  `window.__materialmcReceive(<JSON string literal>)`. The payload is serialised as a JSON string
  literal, so it can never escape into code (`WebView::deliverScript`).
* Messages larger than 16 MiB are dropped; malformed messages are dropped and logged.

## C++ API

`api::ApiRouter` maps method names to handlers:

```cpp
router->addSync("instances.rename", [](const QJsonObject& p) {
    auto* instance = requireInstance(p);                       // INSTANCE_NOT_FOUND
    instance->setName(params::requireNonEmpty(p, "name", 256)); // INVALID_PARAMS
    return ok();
});

router->add("versions.minecraft", [](const QJsonObject& p, const ApiReply& reply) {
    // asynchronous: resolve/reject later, e.g. when a Task finishes
});
```

* Handlers may `throw ApiError{code, message, details}`; the router turns it into an error reply.
  Any other `std::exception` becomes `INTERNAL_ERROR`. C++ exceptions never reach the page raw.
* `ApiReply` is copyable and one-shot, safe to keep across asynchronous work.
* `api::params::*` validates every parameter (type, range, length, file names without separators).
* Long operations return `{ taskId }` immediately and are started on the next loop iteration by
  `TaskTracker`, so the reply always reaches the page before any event of that task.

### Method reference

The authoritative list is `frontend/src/types/methods.ts` (types) — the backend reports its list
through `system.methods`, and the frontend warns about drift at start-up when served by the Vite dev server.

| Group | Methods |
| --- | --- |
| system | `info`, `methods`, `openFolder`, `openUrl`, `copyText`, `saveText`, `icons` |
| instances | `list`, `groups`, `get`, `create`, `copy`, `remove`, `rename`, `setGroup`, `setIcon`, `setNotes`, `getSettings`, `setSettings`, `launch`, `kill` |
| resources | `list`, `setEnabled`, `remove`, `importFiles` (kinds: `mods`, `resourcepacks`, `shaderpacks`, `texturepacks`) |
| worlds | `list`, `remove`, `rename` |
| screenshots | `list`, `remove` |
| logs | `list`, `read` (game log files and crash reports) |
| accounts | `list`, `loginMsa`, `cancelLogin`, `addOffline`, `remove`, `setDefault`, `refresh` |
| versions / java | `versions.minecraft`, `versions.loaders`, `java.list` |
| mods | `search`, `versions`, `install` |
| settings | `get`, `set` |
| tasks | `list`, `cancel`, `clearFinished` |
| console | `get`, `launcherLog` |

### Error codes

`INVALID_PARAMS`, `UNKNOWN_METHOD`, `NOT_FOUND`, `INSTANCE_NOT_FOUND`, `INSTANCE_RUNNING`,
`INSTANCE_NOT_RUNNING`, `INSTANCE_LINKED`, `ACCOUNT_NOT_FOUND`, `NO_ACCOUNT`, `ACCOUNT_NEEDS_REAUTH`,
`ACCOUNT_NEEDS_PROFILE`, `LAUNCH_FAILED`, `TASK_NOT_FOUND`, `TASK_FAILED`, `NETWORK_ERROR`, `UNSUPPORTED`,
`PERMISSION_DENIED`, `CANCELLED`, `IO_ERROR`, `BRIDGE_UNAVAILABLE` (frontend only), `INTERNAL_ERROR`.

## React API

```ts
import { materialmc } from "./api/client";

const instances = await materialmc.instances.list();          // Instance[]
await materialmc.instances.launch({ id, mode: "offline", offlineName: "Steve" });
const { taskId } = await materialmc.instances.create({ name, minecraftVersion: "1.21.1",
                                                       loader: { kind: "fabric", version: "0.16.5" } });
const off = materialmc.on("download.progress", (task) => render(task));   // typed payload

try {
  await materialmc.instances.launch({ id });
} catch (error) {
  if (isMaterialMCError(error) && error.code === "INSTANCE_RUNNING") { ... }
}
```

* Every call returns a `Promise` of the exact result type and rejects with `MaterialMCError`
  (`code`, `message`, `details`).
* `window.materialmc` is the same object, exposed for the inspector.
* Outside the application (plain browser) calls reject with `BRIDGE_UNAVAILABLE` and the UI shows a
  banner — there is no mock data anywhere.
* `hooks/stores.ts` holds the lists several screens share (instances, accounts, tasks). They fetch once
  and then update only from events; there is no polling.

## Events

| Event | Payload | Emitted when |
| --- | --- | --- |
| `instances.changed` | `{}` | instance list/properties/groups/icons changed (debounced 100 ms) |
| `instance.launching` | `{ instanceId }` | `instances.launch` accepted |
| `instance.started` | `{ instanceId }` | the game process is running |
| `instance.stopped` | `{ instanceId, success, reason }` | the game exited or was stopped |
| `instance.launchFailed` | `{ instanceId, error }` | the pipeline stopped before the game ran |
| `instance.consoleRequested` | `{ instanceId }` | "show console" / "show console on error" |
| `minecraft.log` | `{ instanceId, lines: LogLine[] }` | batched game output (≤ 50 ms / 500 lines) |
| `minecraft.crashed` | `{ instanceId, success, reason }` | the game exited abnormally (not on user kill) |
| `download.started/progress/finished/failed` | `TaskInfo` | any tracked backend task (progress ≤ every 150 ms) |
| `account.changed` | `{}` | account list, state or default changed |
| `account.login.status/prompt/finished/failed` | see `types/events.ts` | Microsoft login flow |
| `settings.changed` | `{ keys }` | an allow-listed setting changed (from any UI) |
| `resources.changed` | `{ instanceId, kind }` | a resource folder was rescanned |
| `worlds.changed` | `{ instanceId }` | world list / metadata changed |

Console lines carry a monotonic number `n` per instance. The UI fetches the backlog with
`console.get({ from })` once and then appends events; numbering survives the `LogModel` ring buffer
and relaunches, so nothing is duplicated or lost and full logs are never re-sent.

## Launching

```text
React: materialmc.instances.launch({ id, mode?, offlineName?, accountId?, server?, world? })
  -> InstanceApi::launch          validates, marks "launching", emits instance.launching
  -> Application::launch          unchanged entry point (also used by --launch and the Qt GUI)
  -> LaunchController             unchanged pipeline; with a LaunchInteraction installed it asks
                                  InstanceApi instead of showing dialogs
  -> MinecraftInstance::createLaunchTask -> LaunchTask -> Java process
```

`LaunchInteraction` (launcher/LaunchInteraction.h) is the only change to the launch code: every place
where `LaunchController` used to open a dialog now calls the interaction if one is set. The web
implementation answers from the launch request (mode, player name, account) and records a structured
reason (`NO_ACCOUNT`, `ACCOUNT_NEEDS_REAUTH`, `ACCOUNT_NEEDS_PROFILE`, ...) that is reported with
`instance.launchFailed`. The UI then offers the fix — e.g. a dialog to play offline/demo with a player
name. Account refreshes and update steps run as tracked tasks and appear in *Downloads*.

## Security model

The page is treated as untrusted: a bug such as HTML injection through a mod description must not
give it more power than the fixed API.

1. **No generic entry points.** There is no `execute`, `callCpp`, `eval` or file/process/network
   primitive. Unknown methods fail with `UNKNOWN_METHOD`. Every method validates its parameters.
2. **No paths from the page.** Folders are opened by *name* (`instance.mods`, `logs`, ...); files are
   imported and saved only through native dialogs the user operates; screenshots/world icons/log files
   are addressed by instance id + file name and resolved (and checked) in C++.
3. **Command-line settings need native confirmation.** Java path, JVM arguments, pre-launch / wrapper /
   post-exit commands decide which programs run. `settings.set` and `instances.setSettings` apply them
   only after the user confirms a native Qt dialog showing the new values (`SensitiveChange.h`),
   otherwise `PERMISSION_DENIED`. All other settings are an explicit allow-list with type/range checks.
4. **Links:** `system.openUrl` accepts only `http(s)` URLs with a host. In-page navigation away from
   the app is refused; user-clicked http(s) links open in the system browser.
5. **Origin checks.** Bridge messages are accepted only while the view shows `materialmc://app/`
   (or the configured dev origin). The bootstrap script is injected into the top frame only and
   exposes a frozen, non-configurable object.
6. **Content-Security-Policy** on every `materialmc://app/` response:
   `default-src 'none'; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data: https:;
   font-src 'self' data:; connect-src 'none'; frame-src 'none'; object-src 'none'; base-uri 'none';
   form-action 'none'`, plus `X-Content-Type-Options: nosniff`.
   The page cannot make network requests itself; remote images (mod icons) are the only external loads.
7. **Static files** are served only from the resolved bundle directory; `..` and symlinks leaving it
   are rejected (canonical path check).
8. **WebView hardening:** ephemeral web context (no persistent storage), no popups / new windows,
   permission requests (camera, geolocation, notifications, ...) denied, JavaScript clipboard access
   disabled (`system.copyText` instead), inspector and context menu only in development.
9. **Dev URL only in Debug builds**, and only for loopback hosts (see below). Release builds never load
   anything from the network as UI.

## Building

The normal CMake flow builds everything:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build        # npm ci (only when package-lock.json changes), npm run build, C++ build
cmake --install build --prefix /usr
```

* Requirements: Node.js ≥ 20 with npm, `webkit2gtk-4.1` and `gtk+-3.0` development files
  (Arch: `webkit2gtk-4.1`, Debian/Ubuntu: `libwebkit2gtk-4.1-dev`, Fedora: `webkit2gtk4.1-devel`).
* `frontend/CMakeLists.txt` builds into `frontend/dist/` (type-checking first: `tsc -b && vite build`),
  stages the bundle into `<build>/frontend/` next to the executable and installs it to:
  * Linux/BSD: `share/<LauncherName>/frontend`
  * Windows: `frontend/` next to the executable
  * macOS: `<App>.app/Contents/Resources/frontend`
* At runtime `WebUiHost::findFrontendRoot()` looks in those places (and `<exe dir>/frontend` for the
  build tree). No Node.js or server is needed at runtime.

CMake options:

| Option | Default | Meaning |
| --- | --- | --- |
| `Launcher_ENABLE_WEBUI` | ON if webkit2gtk-4.1 is found (Linux/BSD), OFF elsewhere | build the WebView host and the frontend |
| `Launcher_FRONTEND_DIST_DIR` | empty | use a prebuilt `dist/` instead of running npm (offline / distro builds, e.g. Nix, Flatpak) |
| `Launcher_WEBUI_ALLOW_DEV_URL` | OFF | allow `MATERIALMC_DEV_URL` in non-Debug builds |

## Development mode

```bash
cd frontend
npm install
npm run dev                      # Vite on http://127.0.0.1:5173 with hot reload

# in another terminal, a Debug build of the launcher:
MATERIALMC_DEV_URL=http://localhost:5173 ./build/prismlauncher
```

`MATERIALMC_DEV_URL` is compiled in only for Debug builds (or with `Launcher_WEBUI_ALLOW_DEV_URL=ON`),
accepts only `http(s)://localhost|127.0.0.1|[::1]`, enables the inspector, and whitelists that origin
for the bridge. In dev mode the frontend compares its method list with `system.methods` and logs any
drift in the console.

Other development variables (Debug builds only):

| Variable | Effect |
| --- | --- |
| `MATERIALMC_WEBUI_DEVTOOLS=1` | enable the inspector (right click -> Inspect Element) with the bundled UI |
| `MATERIALMC_FRONTEND_DIR=<dir>` | serve the bundle from another directory |
| `MATERIALMC_WEBUI_ROUTE=/settings` | open a route directly |
| `MATERIALMC_QT_GUI=1` / `--qt-gui` | use the Qt Widgets GUI (all builds) |

Use a separate data directory while developing: `./build/prismlauncher -d /path/to/test-data`.

## Debugging

* **RPC traffic:** `QT_LOGGING_RULES="launcher.webui.api.debug=true"` logs every call and every
  rejected call (code + message) to the launcher log.
* **Page:** enable the inspector (see above); JavaScript errors and CSP violations appear in its console.
* **Blank or unstyled page:** check that `<build>/frontend/index.html` exists (the CMake target
  `Launcher_frontend` builds it) and that the log line `Web UI: loading ...` shows the expected directory.
* **Web UI does not start:** the reason is logged as `Web UI unavailable, falling back to the Qt GUI: ...`.

## Adding an API method

1. Add the method to `MaterialMCMethods` in `frontend/src/types/methods.ts` (and new types next to it).
2. Register it in the matching `launcher/api/*Api.cpp` with `addSync` / `add`; validate all parameters
   with `api::params`, throw `ApiError` for failures, return JSON matching the TypeScript type.
3. Expose it in `frontend/src/api/client.ts` and add it to the `CONTRACT` table there.
4. If it changes state other screens show, emit (or reuse) an event and add it to `types/events.ts`.
5. Never add a method that takes a path, a command, a URL to fetch or a script.

## Known gaps

These flows still use the Qt Widgets GUI (either via `--qt-gui` or because the backend opens the Qt
dialog), and block phase 4:

* Importing modpacks (CurseForge, Modrinth, ATLauncher, FTB, Technic, zip files, URL/drag & drop) —
  imports open the Qt main window with the existing dialogs (`TODO(webui)` in `Application.cpp`).
* Export (zip, `.mrpack`, CurseForge), shortcuts, linked instances.
* Component editor (change Minecraft/loader version, add/remove components) — the web UI shows the
  component list read-only.
* Mod updates, dependency resolution when installing, blocked-mods handling, metadata indexing UI.
* Skins, Minecraft profile creation (`ACCOUNT_NEEDS_PROFILE` points the user to minecraft.net).
* Proxy settings, themes, language, API keys, external tools / profilers, updater UI, news.
* First-run setup wizard (language, Java) is still the Qt wizard.
* Windows (WebView2) and macOS (WKWebView) backends.
* Translations: the web UI is English only.
