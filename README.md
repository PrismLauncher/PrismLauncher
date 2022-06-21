<p align="center">
  <img src="./program_info/polymc-header-black.svg#gh-light-mode-only" alt="PolyMC logo"/>
  <img src="./program_info/polymc-header.svg#gh-dark-mode-only" alt="PolyMC logo"/>
</p>
<br>

PolyMC is a custom launcher for Minecraft that focuses on predictability, long term stability and simplicity.

This is a **fork** of the MultiMC Launcher and not endorsed by MultiMC.
If you want to read about why this fork was created, check out [our FAQ page](https://polymc.org/wiki/overview/faq/).
<br>

# Installation

- All downloads and instructions for PolyMC can be found [here](https://polymc.org/download/)
- Last build status: https://github.com/PolyMC/PolyMC/actions


## Development Builds

There are per-commit development builds available [here](https://github.com/PolyMC/PolyMC/actions). These have debug information in the binaries, so their file sizes are relatively larger.
Portable builds are provided for AppImage on Linux, Windows, and macOS.

For Debian and Arch, you can use these packages for the latest development versions:  
[![polymc-git](https://img.shields.io/badge/aur-polymc--git-blue)](https://aur.archlinux.org/packages/polymc-git/)
[![polymc-git](https://img.shields.io/badge/mpr-polymc--git-orange)](https://mpr.makedeb.org/packages/polymc-git)  
For flatpak, you can use [flathub-beta](https://discourse.flathub.org/t/how-to-use-flathub-beta/2111)

# Help & Support

Feel free to create an issue if you need help. However, you might find it easier to ask in the Discord server.

[![PolyMC Discord](https://img.shields.io/discord/923671181020766230?label=PolyMC%20Discord)](https://discord.gg/xq7fxrgtMP)

For people who don't want to use Discord, we have a Matrix Space which is bridged to the Discord server:

[![PolyMC Space](https://img.shields.io/matrix/polymc:matrix.org?label=PolyMC%20space)](https://matrix.to/#/#polymc:matrix.org)

If there are any issues with the space or you are using a client that does not support the feature here are the individual rooms:

[![Development](https://img.shields.io/matrix/polymc-development:matrix.org?label=PolyMC%20Development)](https://matrix.to/#/#polymc-development:matrix.org)
[![Discussion](https://img.shields.io/matrix/polymc-discussion:matrix.org?label=PolyMC%20Discussion)](https://matrix.to/#/#polymc-discussion:matrix.org)
[![Github](https://img.shields.io/matrix/polymc-github:matrix.org?label=PolyMC%20Github)](https://matrix.to/#/#polymc-github:matrix.org)
[![Maintainers](https://img.shields.io/matrix/polymc-maintainers:matrix.org?label=PolyMC%20Maintainers)](https://matrix.to/#/#polymc-maintainers:matrix.org)
[![News](https://img.shields.io/matrix/polymc-news:matrix.org?label=PolyMC%20News)](https://matrix.to/#/#polymc-news:matrix.org)
[![Offtopic](https://img.shields.io/matrix/polymc-offtopic:matrix.org?label=PolyMC%20Offtopic)](https://matrix.to/#/#polymc-offtopic:matrix.org)
[![Support](https://img.shields.io/matrix/polymc-support:matrix.org?label=PolyMC%20Support)](https://matrix.to/#/#polymc-support:matrix.org)
[![Voice](https://img.shields.io/matrix/polymc-voice:matrix.org?label=PolyMC%20Voice)](https://matrix.to/#/#polymc-voice:matrix.org)

We also have a subreddit you can post your issues and suggestions on:

[r/PolyMCLauncher](https://www.reddit.com/r/PolyMCLauncher/)

# Development

If you want to contribute to PolyMC you might find it useful to join our Discord Server or Matrix Space.

## Building

If you want to build PolyMC yourself, check [Build Instructions](https://polymc.org/wiki/development/build-instructions/) for build instructions.

## Code formatting

Just follow the existing formatting.

In general, in order of importance:

- Make sure your IDE is not messing up line endings or whitespace and avoid using linters.
- Prefer readability over dogma.
- Keep to the existing formatting.
- Indent with 4 space unless it's in a submodule.
- Keep lists (of arguments, parameters, initializers...) as lists, not paragraphs. It should either read from top to bottom, or left to right. Not both.

## Translations

The translation effort for PolyMC is hosted on [Weblate](https://hosted.weblate.org/projects/polymc/polymc/) and information about translating PolyMC is available at https://github.com/PolyMC/Translations

## Download information
To modify download information or change packaging information send a pull request or issue to the website [Here](https://github.com/PolyMC/polymc.github.io/blob/master/src/download.md)

## Forking/Redistributing/Custom builds policy

We don't care what you do with your fork/custom build as long as you follow the terms of the [license](LICENSE) (this is a legal responsibility), and if you made code changes rather than just packaging a custom build, please do the following as a basic courtesy:
- Make it clear that your fork is not PolyMC and is not endorsed by or affiliated with the PolyMC project (https://polymc.org).
- Go through [CMakeLists.txt](CMakeLists.txt) and change PolyMC's API keys to your own or set them to empty strings (`""`) to disable them (this way the program will still compile but the functionality requiring those keys will be disabled).

If you have any questions or want any clarification on the above conditions please make an issue and ask us.

Be aware that if you build this software without removing the provided API keys in [CMakeLists.txt](CMakeLists.txt) you are accepting the following terms and conditions:
 - [Microsoft Identity Platform Terms of Use](https://docs.microsoft.com/en-us/legal/microsoft-identity-platform/terms-of-use)
 - [CurseForge 3rd Party API Terms and Conditions](https://support.curseforge.com/en/support/solutions/articles/9000207405-curse-forge-3rd-party-api-terms-and-conditions)

If you do not agree with these terms and conditions, then remove the associated API keys from the [CMakeLists.txt](CMakeLists.txt) file by setting them to an empty string (`""`).

All launcher code is available under the GPL-3.0-only license.
  
The logo and related assets are under the CC BY-SA 4.0 license.

## Sponsors
Thank you to all our generous backers over at Open Collective! Support PolyMC by [becoming a backer](https://opencollective.com/polymc).

[![OpenCollective Backers](https://opencollective.com/polymc/backers.svg?width=890&limit=1000)](https://opencollective.com/polymc#backers)

Also, thanks to JetBrains for providing us a few licenses for all their products, as part of their [Open Source program](https://www.jetbrains.com/opensource/).

[![JetBrains](https://resources.jetbrains.com/storage/products/company/brand/logos/jb_beam.svg)](https://www.jetbrains.com/opensource/)

Additionally, thanks to the awesome people over at [MacStadium](https://www.macstadium.com/), for providing M1-Macs for development purposes!

<a href="https://www.macstadium.com"><img src="https://uploads-ssl.webflow.com/5ac3c046c82724970fc60918/5c019d917bba312af7553b49_MacStadium-developerlogo.png" alt="Powered by MacStadium" width="300"></a>
