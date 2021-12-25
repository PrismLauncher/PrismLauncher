<p align="center">
  <img src="https://avatars.githubusercontent.com/u/96310119" alt="PolyMC logo"/>
</p>

PolyMC 5
=========

PolyMC is a custom launcher for Minecraft that focuses on predictability, long term stability and simplicity.

This is a **fork** of the MultiMC Launcher and not endorsed by MultiMC. The PolyMC community felt that the maintainer was not acting in the spirit of Free Software so this fork was made. Read "[Why was this fork made?](https://github.com/PolyMC/PolyMC/wiki/FAQ)" on the wiki for more details.

## todo
- [ ] Get a permanent name + icon.
- [ ] Style the logo for different icon styles.
- [ ] Packaging for Linux--Any help packaging for your favorite distro is appreciated!
- [ ] Packaging for MacOS/Windows
- [ ] Stop relying on MultiMC-Hosted metadata services
- [ ] Remove references to MultiMC
- [ ] Change up packaging, remove the install script junk
- [ ] AppImage, Flatpak, .deb, ebuild, and AUR packages
- [ ] Meson
- [x] Long-term solution for the MSA client ID issue
- [x] Figure out a way to switch to GPL.

## Packages
Experimental packages are available for Linux (non-portable) and Windows (portable). Both versions are confirmed to work but the Windows version needs more testing--please volunteer if you use Windows!

- A full list of packages is available [here](https://jenkins.swurl.xyz/job/PolyMC/lastSuccessfulBuild/artifact/).
- Last build status: https://jenkins.swurl.xyz/job/PolyMC/lastBuild/
- [Linux (AMD64) System](https://jenkins.swurl.xyz/job/PolyMC/lastSuccessfulBuild/artifact/lin64-system.tar.zst) - this is a generic system package intended to be used as a base for making distro-specific packages.
- [Windows (32-bit)](https://jenkins.swurl.xyz/job/PolyMC/lastSuccessfulBuild/artifact/win32.zip) - this is a portable package, you can extract it anywhere and run it. This package needs testing.
- MacOS currently does not have any packages. We are still working on setting up MacOS packaging.

## Development
If you want to contribute to PolyMC you might find it useful to join [#development:polymc.org on Matrix](https://matrix.to/#/#development:polymc.org) or join [our Discord server](https://discord.gg/xq7fxrgtMP), which is bridged with the PolyMC Matrix rooms. Thank you!

### Building
If you want to build PolyMC yourself, check [BUILD.md](BUILD.md) for build instructions.

### Code formatting
Just follow the existing formatting.

In general, in order of importance:
* Make sure your IDE is not messing up line endings or whitespace and avoid using linters.
* Prefer readability over dogma.
* Keep to the existing formatting.
* Indent with 4 space unless it's in a submodule.
* Keep lists (of arguments, parameters, initializers...) as lists, not paragraphs. It should either read from top to bottom, or left to right. Not both.

## Translations
TODO

## Forking/Redistributing/Custom builds policy
Do whatever you want, we don't care. Just follow the license. If you have any questions about this feel free to ask in an issue.

## Help & Support
Feel free to create an issue if you need help. However, you might find it easier to ask in the Discord server.

[![PolyMC Discord](https://img.shields.io/discord/923671181020766230?label=PolyMC%20Discord)](https://discord.gg/xq7fxrgtMP)

For people who don't want to use Discord, we have a Matrix Space which is bridged to the discord server. Be sure to enable spaces first (Settings -> Labs -> Spaces), and then you may join the space:

[![PolyMC Space](https://img.shields.io/matrix/polymc:polymc.org?label=PolyMC%20Space&server_fqdn=matrix.polymc.org)](https://matrix.to/#/#polymc:polymc.org)

Matrix's support for spaces is still in development, so if you have issues accessing rooms via the space, then you can join the rooms directly:

[![Support](https://img.shields.io/matrix/support:polymc.org?label=%23support&server_fqdn=matrix.polymc.org)](https://matrix.to/#/#support:polymc.org)
[![Discussion](https://img.shields.io/matrix/discussion:polymc.org?label=%23discussion&server_fqdn=matrix.polymc.org)](https://matrix.to/#/#discussion:polymc.org)
[![Development](https://img.shields.io/matrix/development:polymc.org?label=%23development&server_fqdn=matrix.polymc.org)](https://matrix.to/#/#development:polymc.org)
[![News](https://img.shields.io/matrix/news:polymc.org?label=%23news&server_fqdn=matrix.polymc.org)](https://matrix.to/#/#news:polymc.org)
