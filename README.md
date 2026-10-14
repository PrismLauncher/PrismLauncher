# NeoPrismLauncher

A fork of [Prism Launcher](https://prismlauncher.org) that reworks the user interface.

> This is a **fork** of the MultiMC Launcher and is **not** endorsed by, or affiliated with, Prism Launcher or MultiMC.

## What's shipped

The first milestone (**Tier 1**) is built on the `blocks-ui` branch and rendered headlessly with demo data. The screenshots below are of the launcher compiled from that branch, not mockups.

**Blocks theme** — Minecraft's own UI language drawn as original pixel art: bevelled stone buttons, sunken inventory slots for instances, black text fields, an XP-bar progress meter and purple-framed tooltips. Two variants ship in Settings → Appearance:

- **Blocks (Inventory)** — a dirt backdrop, the library as an inventory grid.
- **Blocks (Deepslate)** — the dark variant, same layout.

It bundles the OFL-licensed [Pixelify Sans](https://github.com/electricity4/Pixelify-Sans) face and original dirt and deepslate textures. The console renders in Minecraft's chat colours, and the background cat is pixelated to match.

**Richer tiles** — the Minecraft version, mod loader and modpack platform are read from the pack file once, cached, and shown under each name (with the platform in the slot corner), instead of only reaching the status bar.

**Library filter bar** — a search field (**Ctrl+F**) across names, versions and loaders, group chips, and sort order moved out of Settings into the bar itself. An empty result now says so ("No instances match \"atm\"") instead of showing a blank grid.

**A primary Play button** — Launch is the one green action. Rename, Change Icon, Change Group, Copy, Create Shortcut and Delete move into a More menu.

**A better editor** — the instance editor opens on Mods instead of an empty console, gains page search, and shares clearer empty states with Settings.

## Why rework the UI

- **Play is not the primary action.** Launch is one row among eleven in the instance toolbar, styled the same as Delete. Double-click launches unless a setting turns it into Edit.
- **Tiles hide what makes instances different.** Minecraft version, loader, pack source, last played and playtime exist on every instance but only reach the status bar.
- **There is no way to search the library.** With thirty or more instances, people scroll and collapse groups. The filtering model already exists; the search box does not.
- **The editor opens on an empty console**, titled "Console window for …", with up to sixteen pages in one flat list that mixes runtime, content and configuration.
- **Five bands of chrome.** Menu bar, main toolbar, instance toolbar, news toolbar and status bar each take a strip, with movable toolbars showing grips by default.
- **Settings have no search** across roughly 130 options on nine pages.

## The plan

The rework is split into three tiers, each including the one before it.

### Tier 1 — the cheapest wins (shipped)

Tile v2 (64 px icon, one-line name, a version and loader line, source and state badges, columns that fill the row), the filter bar, a primary Play button with an overflow menu, the editor opening on Mods with section headers, and Settings search with a designed empty state.

### Tier 2 — a new structure

A navigation rail (Library, Discover, Activity, Settings, Account); a detail panel that absorbs the instance toolbar and status bar; an activity strip and drawer that replace modal progress dialogs; the instance editor regrouped into Overview, Content, Setup and Logs; and a Create hub that starts fresh, browses modpacks across providers, or imports — backed by a task model that keeps progress in the activity strip and turns errors into retryable rows.

### Tier 3 — QML where it pays off

Library and Discover rebuilt in QML inside the Widgets shell for GPU animation and richer cards, at the cost of new Qt Quick dependencies, accessibility parity and a harder native look on KDE and Windows.

### The token layer

Themes can recolour Prism but not reshape it: tile size, icon size, selection style and group headers are hard-coded in three C++ files. A small `ThemeTokens` struct (tile geometry, slot bevel, view textures, display font, cat pixelation) makes those values theme-driven. Its defaults reproduce today's look exactly — Dark and Bright render with **0 differing pixels** compared to an unmodified build — so existing themes keep loading, while custom themes can now use slots, textures and display fonts of their own.

## Visual directions

Five treatments were explored against the same structure and demo instances. Each can be applied to Tier 1 as a theme.

| Direction | Role | Notes |
|---|---|---|
| **Refraction** | Recommended default | The logo's own spectrum as the working palette; Manrope type; a spectral selection ring and a two-pixel spectrum line under the title bar. |
| **Blocks** | Theme pack | Shipped in this fork. Minecraft's UI language, pixel art, Pixelify Sans + Rubik. |
| **Native** | Keep as System | System font, palette and accent; the rework lives in layout and information. |
| **Biome** | View | Cover art per instance — latest screenshot, pack gallery image, or a generated pixel landscape whose sky follows the local time of day. |
| **Cozy** | Theme pack | A sticker-book look around Prism's community art; the cat pack becomes a reactive mascot. |

## Building

To build this fork yourself, follow the [Prism Launcher build instructions](https://prismlauncher.org/wiki/development/build-instructions). The code is C++20 on Qt 6.8 Widgets, with no QML.

## Forking / Redistributing / Custom builds

You are free to fork, redistribute and provide custom builds as long as you follow the terms of the [license](LICENSE). As a basic courtesy, make it clear that your fork is not Prism Launcher and is not endorsed by it, and go through [CMakeLists.txt](CMakeLists.txt) to replace Prism Launcher's API keys with your own or set them to empty strings (`""`) — CurseForge requires derivative works to use their own key.

This fork still needs a full rebrand (name, app ID, logo and desktop files, plus pointing the news feed, bug tracker and updater URLs at its own project).

## License [![License](https://img.shields.io/github/license/PrismLauncher/PrismLauncher?label=License&logo=gnu&color=C4282D)](LICENSE)

All launcher code is available under the GPL-3.0-only license. The logo and related assets are under the CC BY-SA 4.0 license.
