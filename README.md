<p align="center">
<picture>
  <source media="(prefers-color-scheme: dark)" srcset="/program_info/io.github.cakeru.FacetLauncher.logo-darkmode.svg">
  <source media="(prefers-color-scheme: light)" srcset="/program_info/io.github.cakeru.FacetLauncher.logo.svg">
  <img alt="Facet" src="/program_info/io.github.cakeru.FacetLauncher.logo.svg" width="40%">
</picture>
</p>

<p align="center">
  Facet is a custom launcher for Minecraft with a calmer, more streamlined interface.<br />
  <br />Facet is a <b>fork</b> of <a href="https://github.com/PrismLauncher/PrismLauncher">Prism Launcher</a>, which descends from PolyMC and MultiMC.
  <br />It is <b>not</b> affiliated with or endorsed by the Prism Launcher project, PolyMC, MultiMC, Mojang or Microsoft.
</p>

## Status

Facet is in early development. There are no releases yet.

This build differs from Prism Launcher in a few ways while Facet is set up:

- **Microsoft accounts, CurseForge and Imgur uploads are turned off.** Prism Launcher's API keys have been removed, as its fork policy asks. They come back once Facet has its own keys. Offline accounts and Modrinth still work.
- **No news feed or community links.** The news bar, Discord, Matrix and Reddit entries are hidden until Facet has its own.
- **The updater checks this repository's releases.** The macOS (Sparkle) updater is turned off until Facet signs its own builds.
- **Help and wiki links still point to Prism Launcher's wiki**, which covers most of the same features.

## Building

Facet builds the same way as Prism Launcher. Follow Prism Launcher's [build instructions](https://prismlauncher.org/wiki/development/build-instructions).

To use your own Microsoft or CurseForge keys, set `Launcher_MSA_CLIENT_ID` and `Launcher_CURSEFORGE_API_KEY` when configuring with CMake. By doing so you accept the [Microsoft Identity Platform Terms of Use](https://docs.microsoft.com/en-us/legal/microsoft-identity-platform/terms-of-use) and the [CurseForge 3rd Party API Terms and Conditions](https://support.curseforge.com/en/support/solutions/articles/9000207405-curse-forge-3rd-party-api-terms-and-conditions).

## Credits

Facet exists thanks to the work of the [Prism Launcher](https://github.com/PrismLauncher/PrismLauncher), PolyMC and MultiMC contributors. Their copyright notices are kept throughout the source code.

## License [![License](https://img.shields.io/github/license/cakeru/PrismReLauncher?label=License&logo=gnu&color=C4282D)](LICENSE)

All launcher code is available under the GPL-3.0-only license.

The Facet logo and related assets are under the CC BY-SA 4.0 license. Assets inherited from Prism Launcher remain under their original CC BY-SA 4.0 license.
