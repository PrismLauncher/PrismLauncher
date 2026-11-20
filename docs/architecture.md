# Repository architecture

Prism Launcher is primarily a Qt/C++ desktop application centered on `Application`. It also contains a Java bootstrap library that starts Minecraft and, on supported platforms, a separate updater executable.

This is a conceptual architecture description. Some names below are classes, while others are directories or subsystem labels.

## Entry and application shell

`launcher/main.cpp` constructs `Application`, initializes Qt resources, and enters the application event loop.

`launcher/Application.h` and `launcher/Application.cpp` form the application shell and composition root. The global `APPLICATION` macro also makes `Application` act as a service locator throughout the codebase.

`Application` directly owns the principal services:

- `SettingsObject` implementations for global and playtime settings
- Qt's `QNetworkAccessManager`
- `HttpMetaCache`
- `Meta::Index`
- `InstanceList`
- `AccountList`
- `IconList`
- `JavaInstallList`
- `ThemeManager`
- `TranslationsModel`
- An optional `ExternalUpdater`

`Meta::Index` contains shared `Meta::VersionList` objects. `JavaInstallList` and the metadata index are created lazily. `NetJob` is a transient task that borrows the application-owned network manager; it is not an application-owned service.

## Instance and Minecraft domain

The principal ownership chain is:

```text
Application
└── InstanceList
    └── MinecraftInstance
        ├── per-instance SettingsObject
        ├── PackProfile
        │   ├── Component[]
        │   └── resolved LaunchProfile
        ├── resource folder models and WorldList
        └── current LaunchTask
```

`InstanceList` stores `MinecraftInstance` objects using `std::unique_ptr`. `MinecraftInstance` is currently the only loaded implementation of `BaseInstance`; persisted instance types other than empty or `OneSix` are rejected.

`MinecraftInstance::createUpdateTask()` creates transient folder, library, legacy FML library, and asset tasks from `launcher/minecraft/update/`.

Mods, resource packs, texture packs, shader packs, data packs, and worlds are represented by models and resources under `launcher/minecraft/mod/` and `launcher/minecraft/WorldList.*`.

## Accounts and authentication

`Application` owns `AccountList`, which stores shared `MinecraftAccount` objects. Each account owns its active `AuthFlow`, and each flow owns an ordered queue of `AuthStep` objects.

The Microsoft authentication flow consists of OAuth or device-code authorization followed by Xbox, launcher, entitlement, profile, and skin steps. Offline accounts do not run this online flow.

## User interface

The primary windows are:

- `launcher/ui/MainWindow.*`
- `launcher/ui/InstanceWindow.*`

Supporting UI is organized under:

- `launcher/ui/dialogs/`
- `launcher/ui/widgets/`
- `launcher/ui/pages/global/`
- `launcher/ui/pages/instance/`
- `launcher/ui/pages/modplatform/`

`Application` retains raw window pointers while Qt manages window lifetime. It stores one `LaunchController` for each running instance.

## Launch subsystem

The launch flow is:

```text
UI or command line
→ Application::launch()
→ LaunchController
→ MinecraftInstance::createLaunchTask()
→ ordered LaunchStep chain
→ LauncherPartLaunch
→ Java process
```

`Application` owns each active `LaunchController`. The controller borrows its `MinecraftInstance` and the resulting `LaunchTask`. The instance owns the task, and the task owns its ordered launch steps.

Generic steps are under `launcher/launch/steps/`. Minecraft-specific steps are under `launcher/minecraft/launch/`.

## Native-to-Java boundary

`LauncherPartLaunch` starts the configured Java executable with `NewLaunch.jar`, and optionally `NewLaunchLegacy.jar`, on the classpath. The entry point is `org.prismlauncher.EntryPoint`.

The Java helper under `libraries/launcher/` reads the launch description from standard input and invokes Minecraft's main method. The helper and Minecraft run in the same Java process; they are not separate long-lived services.

## Mod platforms

Platform-specific code is organized under:

- `launcher/modplatform/flame/`
- `launcher/modplatform/technic/`
- `launcher/modplatform/modrinth/`
- `launcher/modplatform/atlauncher/`
- `launcher/modplatform/ftb/`
- `launcher/modplatform/legacy_ftb/`
- `launcher/modplatform/import_ftb/`
- `launcher/modplatform/packwiz/`
- `launcher/modplatform/helpers/`

`flame` is the repository name for the CurseForge-oriented implementation. `helpers` and `packwiz` are separate directories.

Common abstractions include `ResourceAPI`, `ModIndex`, `CheckUpdateTask`, `EnsureMetadataTask`, and `InstanceTask`, although not every platform uses all of them.

Capability-based static registration lives in `modplatform/ModPlatformProvider.h` and `modplatform/ModPlatformRegistry.*`. Providers advertise `ProviderCapability` flags (new-instance page, resource types, optional `ResourceAPI`, install task, cache key) and register with `REGISTER_MOD_PLATFORM`. Dynamic plugins are out of scope for this phase; dialogs will iterate the registry in follow-up work instead of hard-coded `pages.append(...)` lists.

## Updater

`ExternalUpdater` is the in-process facade. Its platform implementations include `PrismExternalUpdater` and `MacSparkleUpdater`.

On Windows and Linux, the in-process updater coordinates the separate executable implemented under `launcher/updater/prismupdater/`.
