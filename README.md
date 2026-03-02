<p align="center">
<picture>
  <source media="(prefers-color-scheme: dark)" srcset="/program_info/io.github.houars.Requiem.logo-darkmode.svg">
  <source media="(prefers-color-scheme: light)" srcset="/program_info/io.github.houars.Requiem.logo.svg">
  <img alt="Requiem" src="/program_info/io.github.houars.Requiem.logo.svg" width="40%">
</picture>
</p>

<p align="center">
  Requiem is a custom launcher for Minecraft that allows you to easily manage multiple installations of Minecraft at once.<br />
  <br />This is a <b>fork</b> of <a href="https://prismlauncher.org">Prism Launcher</a> (which itself is a fork of MultiMC) and is <b>not</b> endorsed by or affiliated with either project.
</p>

## Installation

- All releases for Requiem can be found on the [GitHub Releases](https://github.com/Houars/Requiem/releases) page.
- Last build status can be found in the [GitHub Actions](https://github.com/Houars/Requiem/actions) tab (this also includes the pull requests status).

### Development Builds

Please understand that these builds are not intended for most users. There may be bugs, and other instabilities. You have been warned.

Development builds are available through:

- [GitHub Actions](https://github.com/Houars/Requiem/actions) (includes builds from pull requests opened by contributors)

These have debug information in the binaries, so their file sizes are relatively larger.

Prebuilt development builds are provided for **Linux**, **Windows** and **macOS**.

## Community & Support

Feel free to create a [GitHub issue](https://github.com/Houars/Requiem/issues) if you find a bug or want to suggest a new feature.

## Building

If you want to build Requiem yourself, check the [build instructions](https://prismlauncher.org/wiki/development/build-instructions) (based on the upstream Prism Launcher build process).

## Forking/Redistributing/Custom builds policy

You are free to fork, redistribute and provide custom builds as long as you follow the terms of the [license](LICENSE) (this is a legal responsibility), and if you made code changes rather than just packaging a custom build, please do the following as a basic courtesy:

- Make it clear that your fork is not Requiem, Prism Launcher or MultiMC and is not endorsed by or affiliated with any of those projects.
- Go through [CMakeLists.txt](CMakeLists.txt) and change the API keys to your own or set them to empty strings (`""`) to disable them (this way the program will still compile but the functionality requiring those keys will be disabled).

If you have any questions or want any clarification on the above conditions please make an issue and ask us.

Note that if you build this software without removing the provided API keys in [CMakeLists.txt](CMakeLists.txt) you are accepting the following terms and conditions:

- [Microsoft Identity Platform Terms of Use](https://docs.microsoft.com/en-us/legal/microsoft-identity-platform/terms-of-use)
- [CurseForge 3rd Party API Terms and Conditions](https://support.curseforge.com/en/support/solutions/articles/9000207405-curse-forge-3rd-party-api-terms-and-conditions)

If you do not agree with these terms and conditions, then remove the associated API keys from the [CMakeLists.txt](CMakeLists.txt) file by setting them to an empty string (`""`).

## License [![https://github.com/Houars/Requiem/blob/develop/LICENSE](https://img.shields.io/github/license/Houars/Requiem?label=License&logo=gnu&color=C4282D)](LICENSE)

All launcher code is available under the GPL-3.0-only license.

The logo and related assets are under the CC BY-SA 4.0 license.
