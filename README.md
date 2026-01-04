<p align="center">
<picture>
  <source media="(prefers-color-scheme: dark)" srcset="/program_info/org.prismlauncher.PrismLauncher.logo-darkmode.svg">
  <source media="(prefers-color-scheme: light)" srcset="/program_info/org.prismlauncher.PrismLauncher.logo.svg">
  <img alt="Prism Launcher" src="/program_info/org.prismlauncher.PrismLauncher.logo.svg" width="40%">
</picture>
</p>

<p align="center">
  <b>Prism Launcher Cracked</b> is a custom launcher for Minecraft that allows you to easily manage multiple installations of Minecraft at once.
  <br /><br />
  This fork specifically enables <b>full support for offline/cracked accounts</b>, removing the mandatory Microsoft account requirement and ownership checks.
  <br /><br />
  Maintained and Personalized by <b>@Pavle012</b>
</p>

## Features

- **Offline Support**: Add and play with offline accounts without needing a Microsoft account.
- **Bypass Ownership Checks**: Play Minecraft without a Microsoft account that owns the game.
- **Streamlined Setup**: Add your offline account directly from the initial setup wizard.
- **Multiple Instances**: Manage different versions, mods, and settings independently.
- **Open Source**: Licensed under GPL-3.0.

> [!TIP]
> **Skins in Offline Mode**: Since official skins are tied to Microsoft accounts, we recommend using the [CustomSkinLoader](https://modrinth.com/mod/customskinloader) mod or the [SkinsRestorer](https://skinsrestorer.net/) plugin on servers to see your skins while playing offline.

## Installation

- Last build status can be found in the [GitHub Actions](https://github.com/Pavle012/PrismLauncherCracked/actions) tab.

### Development Builds

Prebuilt Development builds are provided for **Linux** and **Windows**.

- [GitHub Actions](https://github.com/Pavle012/PrismLauncherCracked/actions)
- [nightly.link](https://nightly.link/Pavle012/PrismLauncherCracked/workflows/build/develop)

## Community & Support

Feel free to create a GitHub issue if you find a bug or want to suggest a new feature.

## Building

If you want to build Prism Launcher yourself, check the build instructions:

- [Windows](https://prismlauncher.org/wiki/development/build-instructions/windows/)
- [Linux](https://prismlauncher.org/wiki/development/build-instructions/linux/)

## Sponsors & Partners

Thanks to JetBrains for providing licenses for all their products, as part of their [Open Source program](https://www.jetbrains.com/opensource/).

<a href="https://jb.gg/OpenSource">
<picture>
  <source media="(prefers-color-scheme: dark)" srcset="https://www.jetbrains.com/company/brand/img/logo_jb_dos_4.svg">
  <source media="(prefers-color-scheme: light)" srcset="https://resources.jetbrains.com/storage/products/company/brand/logos/jetbrains.svg">
  <img alt="JetBrains logo" src="https://resources.jetbrains.com/storage/products/company/brand/logos/jetbrains.svg" width="40%">
</picture>
</a>

Thanks to Netlify for providing their excellent web services, as part of their [Open Source program](https://www.netlify.com/open-source/).

<a href="https://www.netlify.com"> <img src="https://www.netlify.com/v3/img/components/netlify-color-accent.svg" alt="Deploys by Netlify" /> </a>

## Forking/Redistributing/Custom builds policy

You are free to fork, redistribute and provide custom builds as long as you follow the terms of the [license](LICENSE) (this is a legal responsibility), and if you made code changes rather than just packaging a custom build, please do the following as a basic courtesy:

- Make it clear that your fork is not Prism Launcher and is not endorsed by or affiliated with the Prism Launcher project (<https://prismlauncher.org>).
- Go through [CMakeLists.txt](CMakeLists.txt) and change Prism Launcher's API keys to your own or set them to empty strings (`""`) to disable them (this way the program will still compile but the functionality requiring those keys will be disabled).

If you have any questions or want any clarification on the above conditions please make an issue and ask us.

If you are just building Prism Launcher for your distribution, please make sure to set the `Launcher_BUILD_PLATFORM` to a slug representing your distribution. Examples are `archlinux`, `fedora` and `nixpkgs`.

Note that if you build this software without removing the provided API keys in [CMakeLists.txt](CMakeLists.txt) you are accepting the following terms and conditions:

- [Microsoft Identity Platform Terms of Use](https://docs.microsoft.com/en-us/legal/microsoft-identity-platform/terms-of-use)
- [CurseForge 3rd Party API Terms and Conditions](https://support.curseforge.com/en/support/solutions/articles/9000207405-curse-forge-3rd-party-api-terms-and-conditions)

If you do not agree with these terms and conditions, then remove the associated API keys from the [CMakeLists.txt](CMakeLists.txt) file by setting them to an empty string (`""`).

## License [![https://github.com/Pavle012/PrismLauncherCracked/blob/develop/LICENSE](https://img.shields.io/github/license/Pavle012/PrismLauncherCracked?label=License&logo=gnu&color=C4282D)](LICENSE)

All launcher code is available under the GPL-3.0-only license.

The logo and related assets are under the CC BY-SA 4.0 license.
