# Architecture improvement opportunities

## Summary

Prism Launcher has useful leaf-level abstractions for tasks, pages, instances, and resource APIs. Its main constraint is centralized composition: global service access and hard-coded provider and workflow construction concentrate unrelated changes in a few large classes.

Because this is a desktop application, scalability primarily means:

- Handling large local instance and account collections
- Bounding concurrent network and background work
- Supporting more providers and resource types
- Allowing the codebase and contributor count to grow safely

The recommended approach is incremental. A wholesale rewrite would carry more risk than benefit.

## Highest priorities

### P0: Introduce explicit application-service dependencies

`Application` owns most core services and exposes them through the global `APPLICATION` macro. This creates hidden dependencies in UI, domain, launch, metadata, and provider code. `MainWindow.cpp` alone contains approximately 100 `APPLICATION->` uses.

Introduce narrow interfaces such as:

- `LauncherSettings`
- `NetworkContext`
- `MetadataRepository`
- `InstanceRepository`
- `AccountRepository`

Keep `Application` as the composition root and compatibility facade while migrating one subsystem at a time. New components should receive only the interfaces they require through constructors.

Expected benefits:

- Domain behavior can be tested without constructing `QApplication`
- Dependencies and ownership become visible
- Services can be replaced without editing unrelated consumers
- Module boundaries become enforceable

Avoid replacing `APPLICATION` with another general-purpose service locator.

### P0: Split workflow orchestration from `Application` and `MainWindow`

`Application.cpp` is approximately 2,082 lines, while `MainWindow.cpp` is approximately 1,795 lines. They coordinate startup, settings, imports, updates, accounts, launches, dialogs, and window lifetime.

Extract use-case-oriented coordinators:

- `StartupCoordinator`
- `ImportCoordinator`
- `InstanceLaunchService`
- `InstanceActionController`
- `UpdateCoordinator`

Windows should bind view state and dispatch commands. They should not parse protocols, perform provider-specific downloads, mutate caches, or coordinate domain workflows.

`MainWindow::processURLs()` is a strong first extraction candidate because it currently combines URL parsing, OAuth dispatch, provider requests, JSON parsing, downloads, instance lookup, installation, and dialogs.

### P0: Register mod-platform and authentication providers

Provider construction is centralized:

- `NewInstanceDialog::getPages()` directly creates each platform page
- Resource dialogs branch between provider implementations
- `Application::Capability` contains provider-specific flags
- Account persistence, UI, and `AuthFlow` switch on a closed `AccountType` enum

Add a capability-based `ModPlatformDescriptor` containing:

- Stable serialized identifier
- Supported resource and pack types
- API factory
- New-instance and resource-page factories
- Import and URL handlers
- Managed-pack integration
- Authentication and rate-limit requirements

Add an `AuthProvider` contract containing:

- Stable account-type identifier
- Persistence and migration support
- Login and refresh step factories
- Session population
- Account-creation UI metadata

Use static registration initially. Dynamic plugins are unnecessary unless distribution requirements later demand them.

Skeleton: `ModPlatformProvider` (capability flags, page/install/`ResourceAPI` factories, cache key) and `ModPlatformRegistry` with `REGISTER_MOD_PLATFORM` — see `launcher/modplatform/ModPlatformRegistry.h`.

### P0: Add a global, host-aware work scheduler

The configured download concurrency is applied independently by each `NetJob`. Multiple simultaneous jobs can therefore each consume the full allowance. `ConcurrentTask` also retains task objects and progress state until the containing task is destroyed.

Route network work through a shared scheduler supporting:

- A global concurrency limit
- Per-host and per-provider limits
- Foreground and background priorities
- Cancellation
- Exponential backoff with jitter
- Compact completed-task summaries

This bounds aggregate resource use and prevents retries from amplifying provider outages.

## Scalability improvements

### Build an indexed, incremental instance catalog

`InstanceList` currently:

- Scans every configured root during discovery
- Reconciles the complete result after filesystem changes
- Loads the catalog synchronously during startup
- Uses linear searches for IDs, UUIDs, managed names, and pointer-to-row lookup

Maintain indexes for:

- Instance ID
- UUID
- Managed pack name
- Instance pointer to model row

Debounce filesystem notifications and reconcile only affected roots or paths. Load lightweight catalog metadata first and instantiate full `MinecraftInstance` objects lazily.

Incremental reconciliation must preserve behavior for duplicate IDs, external renames, unavailable roots, and symlinks.

### Make account scheduling and persistence scale-aware

Account lookup is linear, refreshes are serial with a fixed delay, and account changes rewrite the complete JSON document.

Recommended changes:

- Index accounts by internal ID and profile ID
- Debounce and coalesce persistence
- Write atomically
- Replace fixed delays with provider-aware rate policies and jitter
- Preserve serial refresh where provider requirements demand it

### Separate build targets by architectural boundary

Most launcher code is compiled into one large target, and tests link the same target. As the project and contributor count grow, this increases rebuild time and permits accidental cross-layer dependencies.

Incrementally extract targets for:

- Task and networking infrastructure
- Instance and account domains
- Provider adapters
- UI

Do this after introducing service interfaces so target extraction does not merely expose unresolved circular dependencies.

## Maintainability improvements

### Clarify task lifecycle and ownership

Task APIs combine Qt objects, asynchronous completion signals, restartable state, cancellation, and several ownership styles. The repository uses `std::unique_ptr`, `shared_qobject_ptr`, Qt parent ownership, and raw observer pointers.

Define:

- One terminal result path for success, failure, and cancellation
- Explicit cancellation semantics
- A clear rule for restartability
- Ownership conventions for returned and retained tasks
- Observer-pointer conventions such as `QPointer` where Qt-owned objects may disappear

Expand task tests to cover cancellation, failure, duplicate completion, destruction, and event-loop shutdown.

### Consolidate resource-page workflows

Mod, resource-pack, and data-pack pages independently implement similar download, update, progress-dialog, warning, and model-refresh flows.

Extract a composed resource-page controller or helper parameterized by:

- Resource terminology
- Loader and version constraints
- Dialog factories
- Update and download strategies

Prefer composition over a deep widget inheritance hierarchy.

### Return typed, owning provider results

Parts of `ResourceAPI` return `std::pair<Task::Ptr, QByteArray*>`, coupling task lifetime to raw response storage. Provider implementations also duplicate parsing and update-candidate policy.

Introduce a typed asynchronous `ProviderResult<T>` that owns its response and has consistent success, failure, and cancellation behavior. Centralize only confirmed-common update planning while retaining provider-specific request and parsing strategies.

### Move settings and persistence behind schemas and repositories

`Application.cpp` registers approximately 100 settings, while account and group persistence are implemented directly in model classes.

Introduce versioned schemas and repositories with:

- Explicit defaults and validation
- Atomic writes
- Versioned migrations
- Background loading where appropriate
- Test fixtures for old formats

## Extensibility improvements

### Add an instance-type registry

`BaseInstance` defines a polymorphic contract, but `InstanceList` stores `MinecraftInstance`, rejects other persisted instance types, and constructs `MinecraftInstance` directly. `LaunchTask` also depends on `MinecraftInstance`.

Introduce an `InstanceTypeRegistry` keyed by the persisted type. Each registration should provide:

- Instance loader and factory
- Page contributions
- Launch and update pipeline builders
- Serialization and migration behavior

Migrate catalog APIs toward `BaseInstance` incrementally. Persisted `InstanceType` compatibility must be protected by tests.

### Extract named workflow pipeline builders

`LaunchTask` already supports ordered steps, but `MinecraftInstance::createLaunchTask()` manually assembles the full sequence. `AuthFlow` similarly constructs the Microsoft step sequence directly.

Create builders with named phases, for example:

- `pre-metadata`
- `pre-update`
- `pre-launch`
- `game`
- `post-exit`

Instance types and services may contribute steps to defined phases. Ordering and reverse finalization must remain deterministic; unrestricted plugin ordering would be unsafe.

### Register page contributions contextually

`BasePage` and `GenericPageProvider` are useful foundations, but global, instance, and provider pages are still assembled centrally.

Allow registered features to contribute page factories based on context and capabilities. Page factories should receive narrow service dependencies rather than use `APPLICATION`.

## Recommended sequence

### Phase 1: Establish safety and seams

1. Add characterization tests for instance loading, launch composition, imports, and task cancellation.
2. Introduce narrow service interfaces while keeping `Application` as the composition root.
3. Add ID and UUID indexes without changing instance discovery behavior.

### Phase 2: Remove central composition bottlenecks

1. Add the global work scheduler.
2. Add mod-platform and authentication provider registries.
3. Extract import orchestration from `MainWindow`.
4. Introduce launch and authentication pipeline builders.

### Phase 3: Improve large-library and organizational scaling

1. Make filesystem reconciliation incremental and asynchronous.
2. Introduce versioned repositories and atomic persistence.
3. Add the instance-type registry after catalog APIs safely operate on `BaseInstance`.
4. Split build targets along the resulting dependency boundaries.

## Success criteria

- A new modpack provider requires provider-local implementation and one registration.
- Core workflows can be tested without constructing application windows.
- Instance lookup is indexed by ID and UUID.
- Filesystem changes do not force full catalog reloads.
- Aggregate and per-host network limits hold across simultaneous jobs.
- Launch-pipeline ordering and finalization are covered by contract tests.
- Existing settings, accounts, and instance formats migrate without data loss.
