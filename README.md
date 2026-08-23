# Cloudy Launcher

<p align="center">
  <img src="/program_info/org.cloudylauncher.CloudyLauncher_256.png" alt="Cloudy Launcher" width="180">
</p>

<p align="center">
  <strong>A modern Minecraft launcher built on the proven Prism Launcher foundation.</strong><br>
  A focused desktop experience for instances, mods, resources, accounts, files, and safe recovery.
</p>

<p align="center">
  <a href="https://github.com/manusportalgpt-beep/Cloudy-Launcher/actions"><img src="https://img.shields.io/github/actions/workflow/status/manusportalgpt-beep/Cloudy-Launcher/build.yml?label=build" alt="Build status"></a>
  <a href="https://img.shields.io/badge/license-GPL--3.0-blue.svg"><img src="https://img.shields.io/badge/license-GPL--3.0-blue.svg" alt="GPL-3.0 License"></a>
</p>

## What is Cloudy Launcher?

Cloudy Launcher is an independent open-source fork of [Prism Launcher](https://github.com/PrismLauncher/PrismLauncher). It keeps the mature launcher, account, metadata, download, and Minecraft instance foundations while evolving the product around a clearer, safer desktop workflow.

Cloudy is **not affiliated with or endorsed by Prism Launcher**. Prism Launcher, MultiMC, and PolyMC copyrights and license notices remain part of the upstream codebase where applicable.

## Product direction

- preserve reliable upstream functionality before changing presentation;
- make instances the center of the product;
- provide one coherent navigation model with Sidebar and compact Notch Panel modes;
- treat mod and resource updates as recoverable operations with snapshots and rollback;
- keep authentication data outside presentation state and never expose tokens in logs or URLs;
- support offline-first access to local instances, files, logs, and installed content;
- use restrained, purposeful desktop UI instead of decorative effects.

The migration is delivered incrementally. Features are advertised as available only after implementation and verification in the codebase.

## Building

Cloudy Launcher follows the upstream Prism Launcher build requirements and CMake workflow. See the upstream [build instructions](https://prismlauncher.org/wiki/development/build-instructions/) for platform dependencies, then configure and build this repository from the `develop` branch.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

## Contributing

Issues and pull requests are welcome. Keep changes focused, preserve existing license and third-party notices, and verify launcher functionality when changing UI or branding.

## License

Cloudy Launcher is distributed under the GNU General Public License, version 3 or later. See [LICENSE](LICENSE).

This repository contains code originating from Prism Launcher, MultiMC, PolyMC, and other third-party components. Their respective notices and licenses are retained in the repository.

## Upstream

- Prism Launcher source: https://github.com/PrismLauncher/PrismLauncher
- Cloudy Launcher source: https://github.com/manusportalgpt-beep/Cloudy-Launcher
