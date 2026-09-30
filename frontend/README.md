# MaterialMC frontend

The user interface of MaterialMC: a React 19 + TypeScript (strict) application built with Vite and
rendered by the system WebView (WebKitGTK on Linux). It contains no launcher logic — everything goes
through the typed backend API `window.materialmc`. Architecture, IPC, security model and the full API
reference: [docs/react-webview.md](../docs/react-webview.md).

## Scripts

```bash
npm install          # once
npm run dev          # dev server on http://127.0.0.1:5173 (use with MATERIALMC_DEV_URL, see below)
npm run typecheck    # tsc, no output
npm run build        # type-check + production bundle in dist/
```

You normally do not run `npm run build` yourself: `cmake --build` does it (target `Launcher_frontend`)
and installs `dist/` with the launcher.

## Running against the real backend

The UI only works inside MaterialMC — opened in a normal browser it shows "Backend not connected"
and no data. Start a Debug build of the launcher pointed at the dev server:

```bash
npm run dev
MATERIALMC_DEV_URL=http://localhost:5173 ../build/prismlauncher -d ../build/dev-data
```

Hot reload works as usual. `MATERIALMC_WEBUI_DEVTOOLS=1` enables the WebKit inspector for the bundled
UI as well.

## Layout

```text
src/
├── types/        contract with the C++ API (launcher/api): methods.ts, events.ts, domain types
├── api/          bridge.ts (transport) and client.ts (window.materialmc)
├── hooks/        useQuery / useEvent, event-driven shared stores, waitForTask
├── components/   Layout, dialogs, menus, VirtualList, ConsoleView, ResourceList, ...
├── pages/        one component per route
└── styles/       global.css (design tokens, light/dark via prefers-color-scheme)
```

## Rules

* **No mock data.** Every screen shows backend data or an explicit empty/error state. If the backend
  lacks a feature, leave a `TODO(webui)` and do not render a fake control.
* **Typed calls only.** Add a method to `types/methods.ts` and `api/client.ts` (and its `CONTRACT`
  table) together with the C++ handler. Avoid `any`; the backend payloads have real types.
* **Handle errors.** Calls reject with `MaterialMCError` (`code`, `message`); show them with
  `useToasts().showError()` or `<ErrorBanner>`.
* **No polling.** Subscribe to events (`useEvent`, the stores in `hooks/stores.ts`). Long lists use
  `VirtualList`.
* **The page has no network access** (CSP `connect-src 'none'`); everything goes through the backend.
