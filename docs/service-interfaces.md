# Service interfaces: ownership and injection (R1.1 / R1.2)

This note covers the narrow service contracts introduced for R1 and the rules for owning and injecting them. Call-site migration is intentionally deferred to later R1 sub-issues.

## Contracts

| Interface | Purpose | Current wrapper |
| --- | --- | --- |
| `LauncherSettings` | Global and playtime settings access | `SettingsObjectLauncherSettings` |
| `NetworkContext` | Shared `QNetworkAccessManager`, `HttpMetaCache`, user-agent, CurseForge key, Modrinth token | `SharedNetworkContext` |
| `InstanceCatalog` | Instance lookup and lifecycle operations | `InstanceListCatalog` |
| `AccountService` | Account list, default account, refresh entry points | `AccountListService` |

Headers live under `launcher/services/`.

## Ownership and lifetime (R1.2)

`Application` is the composition root. At startup it constructs and owns:

1. The concrete backing objects: `SettingsObject` (global + playtime), `QNetworkAccessManager`, `HttpMetaCache`, `InstanceList`, and `AccountList` (unchanged).
2. The four wrappers as `std::unique_ptr` members, created in `Application`'s constructor after those backing objects exist:
   - `SettingsObjectLauncherSettings` → `m_launcherSettings`
   - `SharedNetworkContext` → `m_networkContext`
   - `InstanceListCatalog` → `m_instanceCatalog`
   - `AccountListService` → `m_accountService`

Wrappers hold non-owning pointers to the backing objects. They must not outlive those objects. Member declaration order places the wrappers after the concrete members so wrappers are destroyed first on shutdown.

Accessors for injection (return interface pointers):

| Accessor | Returns |
| --- | --- |
| `launcherSettings()` | `LauncherSettings*` |
| `networkContext()` | `NetworkContext*` |
| `instanceCatalog()` | `InstanceCatalog*` |
| `accountService()` | `AccountService*` |

Legacy accessors `settings()`, `playtimeSettings()`, `network()`, `metacache()`, `instances()`, and `accounts()` remain for unmigrated call sites. Do not expand the `APPLICATION` macro with new responsibilities; prefer these interface accessors when migrating.

Wrappers do not introduce a second service locator. Do not replace `APPLICATION` with another global that returns these interfaces.

## Injection rules

- New and migrated code takes the narrow interface it needs via constructor parameter (or setter only when Qt parent ownership forces a two-phase init).
- Prefer the smallest interface: settings-only code gets `LauncherSettings*`, not `Application*`.
- Do not pass `Application*` into domain, net, or provider types when one of these contracts suffices.
- Transitional methods such as `LauncherSettings::settingsObject()`, `InstanceCatalog::model()`, and `AccountService::model()` exist so Qt models and `SettingsObject` signal wiring can migrate incrementally. New non-UI logic should avoid them.
- Tests may construct wrappers around fakes or temporary `INISettingsObject` / list instances without constructing `QApplication` as a service locator.

## MainWindow migration (R1.6)

`MainWindow` receives `LauncherSettings`, `InstanceCatalog`, `AccountService`, and `NetworkContext` through its constructor; `Application::showMainWindow()` passes its own wrappers. All settings, playtime, instance, account, network, and metadata-cache access in `MainWindow.cpp` goes through those pointers. Qt model binding and `InstanceList` / `AccountList` signal connections use the transitional `model()` accessors.

`InstanceCatalog` gained `deleteGroup`, `renameGroup`, `trashedSomething`, and `undoTrashInstance` so group and trash actions do not need `model()`.

`APPLICATION` uses in `MainWindow.cpp` went from 109 to 31. The remaining uses are application-level UI concerns with no narrow contract yet:

| Remaining use | Call sites | Owner |
| --- | --- | --- |
| `launch()`, `kill()` | double-click, Launch, Kill actions | R2.1 LaunchCoordinator (#15) |
| `oauthReplyRecieved` signal | `processURLs()` OAuth callback URLs (forwarded to auth via `OAuthCallbackRelay` since R1.4) | R2.3 Account / OAuth coordinator (#17) |
| `ShowGlobalSettings()`, `globalSettingsApplied` | Settings and Manage Accounts actions, settings-closed refresh | R2.4 thin Application/MainWindow (#18) |
| `showInstanceWindow()`, `showLogWindow()`, `closeCurrentWindow()`, `quit()` | Edit Instance, View Log, close and quit shortcuts | R2.4 thin Application/MainWindow (#18) |
| `updaterEnabled()`, `updatesAreAllowed()`, `updater()`, `triggerUpdateCheck()` | update menu visibility and Check for Updates | R2.4 thin Application/MainWindow (#18) |
| `themeManager()`, `currentCatChanged` signal | theme menu, theme/cat-pack folder actions, cat background | UI layer; stays with the app target (R7.3, #35) |
| `icons()` | instance icon toolbar, icon-updated signal, icons folder | R1 follow-up (#1): needs an icon repository contract |
| `javaPath()` | View Java Folder action | R1 follow-up (#1): candidate for `LauncherSettings` |

## Out of scope for R1.1 / R1.2

- Rewriting `APPLICATION->…` call sites (R1.3+)
- Extracting `AuthFlow` construction into `AccountService`
- Moving API keys / capability flags onto `NetworkContext` (API keys done in R1.3)
- Splitting CMake targets by layer (R7)

## Flame / Modrinth migration (R1.3)

- `NetworkContext` exposes `flameApiKey()` and `modrinthApiToken()`. `SharedNetworkContext` owns the lookup (user override, else build-time key); `Application::getFlameAPIKey()` / `getModrinthAPIToken()` delegate to it.
- `ResourceAPI`, `FlameAPI`, and `ModrinthAPI` take a `NetworkContext*` in their constructor. The `FlameAPI::get()` / `ModrinthAPI::get()` singletons are gone, and the former static network calls (`getFiles`, `matchFingerprints`, `currentVersions`, changelog/description, …) are instance methods.
- `Net::ApiHeaderProxy` reads credentials from the `NetworkContext` it is given. `Net::ApiRequest` has overloads that take a `NetworkContext*`; provider code uses those. The legacy overloads (`Request::Option::AddAPIHeaders`) still resolve the context from `Application` inside `net/Request.cpp`.
- Flame/Modrinth tasks receive dependencies through their constructors: `FlameCheckUpdate`, `ModrinthCheckUpdate`, `Flame::FileResolvingTask`, `EnsureMetadataTask`, `FlamePackExportTask`, `ModrinthPackExportTask`, `FlameCreationTask`, `ModrinthCreationTask`, plus `FTB::PackInstallTask` and `GetModDependenciesTask` because they build Flame/Modrinth clients. Tasks hold their API client as a member, not a local, because `ResourceAPI` callbacks capture `this`.
- `Application` owns one `FlameAPI` and one `ModrinthAPI` (`flameAPI()` / `modrinthAPI()`) for UI pages that keep a long-lived `const ResourceAPI*`. UI and `InstanceImportTask` remain the edge that reads `APPLICATION->networkContext()` / `launcherSettings()`.
- Result: lines using `APPLICATION` under `launcher/modplatform/` went from 42 to 15, and files including `Application.h` there went from 14 to 5. No Flame/Modrinth code reads network, API keys, or settings through `APPLICATION`. The three remaining Flame/Modrinth uses are `instances()` and `metadataIndex()` (instance and metadata domain, not network/settings). The rest are in ATLauncher, Technic, and legacy FTB.
- `tests/ModPlatformInjection_test.cpp` checks header injection and job creation with a fake `NetworkContext` and no `Application`.

## Auth step migration (R1.4)

- `AuthContext` (`minecraft/auth/AuthContext.h`) bundles the auth pipeline's dependencies: `NetworkContext*`, `LauncherSettings*` (for `MSAClientIDOverride`), an `OAuthCallbackRelay*` for custom-scheme OAuth redirects, the portable flag, and the desktop file name. `msaClientId()` resolves the override, else the build-time ID; `Application::getMSAClientID()` delegates to it.
- `Application::authContext()` builds the context from its own services. `OAuthCallbackRelay` is owned by `Application` and forwards `oauthReplyRecieved`, so `MainWindow` still emits the same signal.
- `AuthStep` takes a `NetworkContext*`; every step builds its `NetJob` (or the OAuth flow's network manager) on it. `MSAStep` takes the whole `AuthContext`; `MSADeviceCodeStep` takes the network and an already-resolved client ID. With no relay, `MSAStep` uses the loopback HTTP redirect.
- `AuthFlow`, `MinecraftAccount::login()` / `refresh()`, and `AccountList` take an `AuthContext`. `Application` now builds `AccountList` (and `AccountListService`) after the service wrappers, because `fillQueue()` starts a refresh immediately. UI dialogs and `LaunchController` pass `APPLICATION->authContext()`.
- `Net::Request` falls back to the build-time user agent and the default transfer timeout when there is no `Application`, so requests run in plain `QCoreApplication` tests.
- `Net::Request` again records `Succeeded` on a successful download. The `std::expected` sink refactor (`bb27d2e13`) had dropped that, so `wasSuccessful()` was false for good responses and the MSA device-code step rejected them. The device-code test covers this.
- Result: `launcher/minecraft/auth/` has no `APPLICATION` uses or `Application.h` includes (previously 14 lines across 9 files).
- `tests/AuthSteps_test.cpp` runs the offline flow, the client-ID-changed refresh path, `XboxUserStep`, and `MSADeviceCodeStep` against a fake `NetworkContext` whose network manager returns canned replies.

## Instance / profile migration (R1.5)

- `InstanceServices` bundles what an instance needs to load and launch: `LauncherSettings*`, `NetworkContext*`, `IconStore*`, `Meta::Index*`, and `const LauncherRuntimeInfo*` (detected OpenAL/GLFW/SDL paths and MangoHud). Pointers are non-owning. A null pointer means that service is absent, which is how headless tests construct an instance.
- `Application::instanceServices()` builds the bundle. The meta cache, settings wrapper, network wrapper, and icon store are created before `InstanceList::loadList()`, because loading an instance copies the pointers. `LauncherRuntimeInfo` is updated in place by `detectLibraries()` and `updateCapabilities()`, so instances see the detected libraries at launch time.
- `InstanceList` and `InstanceTask` carry the bundle. Every `MinecraftInstance` constructor takes it: instance load, vanilla creation, and the Flame, Modrinth, FTB, legacy FTB, import FTB, ATLauncher, and Technic install tasks.
- `MinecraftInstance` uses it for native-library arguments, the libraries directory, game-time formatting, mod-metadata indexing, the launch icon, and MangoHud. `PackProfile` no longer includes `Application.h`; components and `ComponentUpdateTask` read the metadata index from the instance. Library, asset, and legacy FML update tasks build their `NetJob`s on the injected network and read URL overrides from the injected settings.
- `populateLaunchMenu` calls the injected `LaunchCoordinator` for launch and the profiler list. Java download and window show/hide go through `JavaEnvironment` and `LauncherWindows` (see below). Account claim stays on `Application`.
- `tests/InstanceServices_test.cpp` checks the injected OpenAL path, the asset URL override, and that a service-less instance still builds arguments.

## Remaining workflow migration (R1)

- `LaunchCoordinator` owns launch and kill. `Application::launch()` / `kill()` delegate to it. Menus, the main window, the instance window, and world/server pages call the coordinator on `InstanceServices`.
- `LauncherWindows` is the window contract (main window, instance window, log window, global settings). `Application` implements it. Launch steps and settings buttons use the pointer instead of `APPLICATION`.
- `JavaEnvironment` exposes the managed Java directory and the detected-install list. Java pages, the setup wizard, and `AutoInstallJava` take it. `JavaInstallList` no longer asks `APPLICATION` for the directory or the task concurrency setting.
- `IconStore::list()` returns the `IconList` the UI already uses as a model. Icon pickers, the instance toolbar, and shortcut creation go through that pointer.
- Metadata download uses `MetaServices` (settings + network) stored on `Meta::Index` and copied onto version lists and versions. `Application::metadataIndex()` binds it. Version lists load the index they belong to instead of `APPLICATION->metadataIndex()`.
- Updater, themes, translations, and startup stay on `Application`.
