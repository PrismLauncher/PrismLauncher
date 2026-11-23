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
