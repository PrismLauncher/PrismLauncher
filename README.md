<p align="center">
<img src="./program_info/org.prismlauncher.PrismLauncher.logo.svg#gh-light-mode-only" alt="Prism Launcher logo" width="50%"/>
<img src="./program_info/org.prismlauncher.PrismLauncher.logo-darkmode.svg#gh-dark-mode-only" alt="Prism Launcher logo" width="50%"/>
</p>

Prism Launcher is a custom launcher for Minecraft that allows you to easily manage multiple installations of Minecraft at once.

This is a **fork** of the MultiMC Launcher and not endorsed by MultiMC.

## Installation

- All downloads and instructions for Prism Launcher can be found [on our website](https://prismlauncher.org/download/).
- Last build status can be found [here](https://github.com/PrismLauncher/PrismLauncher/actions).

### Development Builds

There are development builds available [here](https://github.com/PrismLauncher/PrismLauncher/actions). These have debug information in the binaries, so their file sizes are relatively larger.

Portable builds are provided for on Linux, Windows, and macOS.

For Debian and Arch, you can use these packages for the latest development versions:  
[![prismlauncher-git](https://img.shields.io/badge/aur-prismlauncher--git-blue)](https://aur.archlinux.org/packages/prismlauncher-git/)
[![prismlauncher-git](https://img.shields.io/badge/mpr-prismlauncher--git-orange)](https://mpr.makedeb.org/packages/prismlauncher-git)  
## Help & Support

Feel free to create an issue if you need help. However, you might find it easier to ask in the Discord server.

[![Prism Launcher Discord server](https://discordapp.com/api/guilds/1031648380885147709/widget.png?style=banner3)](https://discord.gg/prismlauncher)

We will also soon be opening up our Matrix channels.
You can already join our Matrix space:

[![PrismLauncher Space](https://img.shields.io/matrix/prismlauncher:matrix.org?label=PrismLauncher%20space)](https://matrix.to/#/#prismlauncher:matrix.org)

We also have a subreddit you can post your issues and suggestions on:

[r/PrismLauncher](https://www.reddit.com/r/PrismLauncher/)

## Building

If you want to build Prism Launcher yourself, check [Build Instructions](https://prismlauncher.org/wiki/development/build-instructions/) for build instructions.

## Translations

The translation effort for PrismLauncher is hosted on [Weblate](https://hosted.weblate.org/projects/prismlauncher/launcher/) and information about translating Prism Launcher is available at <https://github.com/PrismLauncher/Translations>

## Forking/Redistributing/Custom builds policy

We don't care what you do with your fork/custom build as long as you follow the terms of the [license](LICENSE) (this is a legal responsibility), and if you made code changes rather than just packaging a custom build, please do the following as a basic courtesy:

- Make it clear that your fork is not PrismLauncher and is not endorsed by or affiliated with the PrismLauncher project (<https://prismlauncher.org>).
- Go through [CMakeLists.txt](CMakeLists.txt) and change PrismLauncher's API keys to your own or set them to empty strings (`""`) to disable them (this way the program will still compile but the functionality requiring those keys will be disabled).

If you have any questions or want any clarification on the above conditions please make an issue and ask us.

Be aware that if you build this software without removing the provided API keys in [CMakeLists.txt](CMakeLists.txt) you are accepting the following terms and conditions:

- [Microsoft Identity Platform Terms of Use](https://docs.microsoft.com/en-us/legal/microsoft-identity-platform/terms-of-use)
- [CurseForge 3rd Party API Terms and Conditions](https://support.curseforge.com/en/support/solutions/articles/9000207405-curse-forge-3rd-party-api-terms-and-conditions)

If you do not agree with these terms and conditions, then remove the associated API keys from the [CMakeLists.txt](CMakeLists.txt) file by setting them to an empty string (`""`).

## License

All launcher code is available under the GPL-3.0-only license.
  
The logo and related assets are under the CC BY-SA 4.0 license.

## Sponsors

Thanks to JetBrains for providing us a few licenses for all their products, as part of their [Open Source program](https://www.jetbrains.com/opensource/).

[![JetBrains](https://resources.jetbrains.com/storage/products/company/brand/logos/jb_beam.svg)](https://www.jetbrains.com/opensource/)

Thanks to Weblate for hosting our translation efforts.

<a href="https://hosted.weblate.org/engage/prismlauncher/">
<img src="https://hosted.weblate.org/widgets/prismlauncher/-/open-graph.png" alt="Translation status" width="300" />
</a>

Thanks to Netlify for providing us their excellent web services, as part of their [Open Source program](https://www.netlify.com/open-source/)

<a href="https://www.netlify.com"> <img src="https://www.netlify.com/v3/img/components/netlify-color-accent.svg" alt="Deploys by Netlify" /> </a>

Thanks to the awesome people over at [MacStadium](https://www.macstadium.com/), for providing M1-Macs for development purposes!

<a href="https://www.macstadium.com"><img src="https://uploads-ssl.webflow.com/5ac3c046c82724970fc60918/5c019d917bba312af7553b49_MacStadium-developerlogo.png" alt="Powered by MacStadium" width="300"></a>
