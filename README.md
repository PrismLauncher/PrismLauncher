<p align="center">
  <img src="./program_info/polymc-header-black.svg#gh-light-mode-only" alt="PolyMC logo"/>
  <img src="./program_info/polymc-header.svg#gh-dark-mode-only" alt="PolyMC logo"/>
</p>
<br>

PolyMC is a custom launcher for Minecraft that focuses on predictability, long term stability and simplicity.

This is a **fork** of the MultiMC Launcher and not endorsed by MultiMC. The PolyMC community felt that the maintainer was not acting in the spirit of Free Software so this fork was made.
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

we also have a subreddit you can post your issues and suggestions on:

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

Do whatever you want, we don't care. Just follow the license. If you have any questions about this feel free to ask in an issue.

All launcher code is available under the GPL-3 license.
  
[Source for the website](https://github.com/PolyMC/polymc.github.io) is hosted under the AGPL-3 License.

The logo and related assets are under the CC BY-NC-SA 4.0 license.
