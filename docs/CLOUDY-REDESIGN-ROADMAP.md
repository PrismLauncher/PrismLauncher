# Cloudy Launcher redesign roadmap

This document turns the product brief into incremental changes that can be built and verified without discarding the Prism Launcher foundation.

## Navigation contract

Cloudy has two presentation modes backed by the same destinations:

- **Sidebar**: persistent left navigation for Home, Library, Discover, Tools, and System.
- **Notch Panel**: compact top navigation exposing Home, Search, Instances, Mods, Resources, Servers, Account, and Settings.

Navigation state must be independent from instance selection and must not replace existing launcher actions until the replacement surface has parity.

## Surface order

1. **Foundation** — Cloudy identity, application display metadata, resource namespace, theme tokens, and an explicit empty/loading/error state vocabulary.
2. **Home** — last played instance, quick launch, recent instances, update/download summaries, account status, and actionable notices.
3. **Instances** — grid/list modes, search, sorting, groups, favorites, loader/version/mod-count metadata, and overflow actions.
4. **Instance Editor** — Overview, Mods, Resource Packs, Shader Packs, Files, Worlds, Java, Arguments, Logs, Screenshots, Backups, and Snapshots.
5. **Discover** — Mods, Resource Packs, Shader Packs, and Servers with shared filters and cached/offline states.
6. **Recovery** — snapshots, backups, update history, deleted files, restore points, and rollback actions.
7. **System** — Accounts, Appearance/Navigation, Java, launcher settings, and diagnostics.

## Safety rules

- Preserve the existing launch, account, download, instance, and mod-management behavior while each surface is migrated.
- Do not expose a recovery button unless the underlying snapshot or backup operation exists.
- Before mass updates: snapshot, resolve dependencies, download, install, validate, and restore on failure.
- Never claim a crash cause with certainty when it is only inferred from logs.
- Keep tokens out of logs and retain the upstream GPL-3.0 and third-party notices.

## Visual direction

Use the supplied pixel cloud as a restrained identity mark: pale blue and white on a dark neutral canvas. Use hierarchy, spacing, and state changes to communicate—not gradients, glow, glassmorphism, emoji, oversized hero panels, or decorative badges.

## Verification gate

Every migrated surface must pass:

- CMake configure and platform build;
- existing launcher tests;
- launch/list/select/edit instance smoke path;
- account and offline-mode regression checks;
- resource loading check for light and dark themes;
- offline, empty, loading, error, and recovery states;
- Windows packaging workflow before merging the surface.

The current branch contains the foundation only. The next implementation slice is the navigation shell, introduced without removing the existing toolbars until parity is verified.
