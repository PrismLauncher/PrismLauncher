<p align="center">
<picture>
  <img alt="J Launcher" src="/program_info/org.jlauncher.JLauncher.svg" width="40%">
</picture>
</p>

<p align="center">
  J Launcher is an independent open-source Minecraft launcher for managing multiple installations.<br />
  <br />This is the official public J Launcher fork of <a href="https://github.com/PrismLauncher/PrismLauncher">Prism Launcher</a>, which is itself derived from MultiMC.
</p>

## Installation

- Public packages, when available, are listed on the [J Launcher releases page](https://github.com/Jom3a-J/J-Launcher-Minecraft/releases).
- Build and test status is shown in the [J Launcher GitHub Actions](https://github.com/Jom3a-J/J-Launcher-Minecraft/actions) tab.

### Development Builds

Please understand that these builds are not intended for most users. There may be bugs, and other instabilities. You have been warned.

Development builds, if published, are announced through the [J Launcher GitHub Actions](https://github.com/Jom3a-J/J-Launcher-Minecraft/actions) tab. They are not intended for most users and may contain bugs or other instability.

## Support and security

Use the [J Launcher issue tracker](https://github.com/Jom3a-J/J-Launcher-Minecraft/issues) for public bug reports and feature requests. Read the relevant logs first and remove credentials, personal paths, and private server addresses.

Report vulnerabilities privately through [GitHub security advisories](https://github.com/Jom3a-J/J-Launcher-Minecraft/security/advisories/new); do not publish account, token, signing, or release-integrity problems in a public issue.

## Translations

J Launcher currently inherits Prism Launcher's translation project on [Weblate](https://hosted.weblate.org/projects/prismlauncher/launcher/). Translation infrastructure remains upstream-owned until J Launcher publishes its own project.

## Building

If you want to build J Launcher yourself, start with the repository's [contribution guidelines](CONTRIBUTING.md). Prism Launcher's [upstream build documentation](https://prismlauncher.org/wiki/development/build-instructions) remains useful for the inherited build system.

## Forking/Redistributing/Custom builds policy

J Launcher is itself an independent fork of Prism Launcher. You are free to fork, redistribute, and provide custom builds as long as you follow the terms of the [license](LICENSE). If you make code changes, please:

- Make it clear that your fork is not J Launcher and is not endorsed by or affiliated with this project.
- Preserve the Prism Launcher, PolyMC, and MultiMC attribution and license notices.
- Review [CMakeLists.txt](CMakeLists.txt) and use your own service credentials, or leave integrations disabled until you have reviewed them.

If you have questions about these conditions, open a discussion in the [J Launcher issue tracker](https://github.com/Jom3a-J/J-Launcher-Minecraft/issues). Distribution builds should set `Launcher_BUILD_PLATFORM` to a slug representing the distribution.

Any service credentials used in a build are the builder's responsibility and must comply with the applicable service terms, including:

- [Microsoft Identity Platform Terms of Use](https://docs.microsoft.com/en-us/legal/microsoft-identity-platform/terms-of-use)
- [CurseForge 3rd Party API Terms and Conditions](https://support.curseforge.com/en/support/solutions/articles/9000207405-curse-forge-3rd-party-api-terms-and-conditions)

Do not commit private credentials. Set an integration to an empty string in [CMakeLists.txt](CMakeLists.txt) when it is not configured for your build.

## License [![https://github.com/Jom3a-J/J-Launcher-Minecraft/blob/develop/LICENSE](https://img.shields.io/github/license/Jom3a-J/J-Launcher-Minecraft?label=License&logo=gnu&color=C4282D)](LICENSE)

All launcher code is available under the GPL-3.0-only license. J Launcher preserves the upstream Prism Launcher, PolyMC, and MultiMC history, copyright notices, and license terms.

The inherited logo and related assets are under the CC BY-SA 4.0 license where indicated by their accompanying notices.
