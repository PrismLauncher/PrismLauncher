<p align="center">
  <img src="./program_info/polymc-header-black.svg#gh-light-mode-only" alt="PolyMC logo"/>
  <img src="./program_info/polymc-header.svg#gh-dark-mode-only" alt="PolyMC logo"/>
</p>
<br>

PolyMC is a custom launcher for Minecraft that focuses on predictability, long term stability and simplicity.

This is a **fork** of the MultiMC Launcher and not endorsed by MultiMC. The PolyMC community felt that the maintainer was not acting in the spirit of Free Software so this fork was made.
<br>

# Installation
- All packages (archived by version) can be found [here](https://packages.polymc.org/) ([latest](https://packages.polymc.org/latest)).
- Last build status: https://jenkins.polymc.org/job/PolyMC/lastBuild/

## 🐧 Linux

### <img src="https://www.vectorlogo.zone/logos/linuxfoundation/linuxfoundation-icon.svg" height="20" alt=""/> Cross-distro packages

<a href='https://flathub.org/apps/details/org.polymc.PolyMC'><img width='240' alt='Download on Flathub' src='https://flathub.org/assets/badges/flathub-badge-en.png'/></a>

<a href="https://packages.polymc.org/latest/appimage/PolyMC-latest-x86_64.AppImage"><img src="https://docs.appimage.org/_images/download-appimage-banner.svg" width="240" alt="Download as AppImage" /></a>

- [AppImage SHA256](https://packages.polymc.org/latest/appimage/PolyMC-latest-x86_64.AppImage.sha256)

### <img src="https://www.vectorlogo.zone/logos/archlinux/archlinux-icon.svg" height="20"/> Arch Linux

There are several AUR packages available:

[![polymc](https://img.shields.io/badge/aur-polymc-blue)](https://aur.archlinux.org/packages/polymc/)
[![polymc-bin](https://img.shields.io/badge/aur-polymc--bin-blue)](https://aur.archlinux.org/packages/polymc-bin/)
[![polymc-git](https://img.shields.io/badge/aur-polymc--git-blue)](https://aur.archlinux.org/packages/polymc-git/)

```sh
# stable source package:
yay -S polymc
# stable binary package:
yay -S polymc-bin
# latest git package:
yay -S polymc-git
```

### <img src="https://www.vectorlogo.zone/logos/debian/debian-icon.svg" height="20" /> Debian

We use [makedeb](https://docs.makedeb.org/) for our Debian packages.
Several MPR packages are available:

[![polymc](https://img.shields.io/badge/mpr-polymc-orange)](https://mpr.makedeb.org/packages/polymc)
[![polymc-bin](https://img.shields.io/badge/mpr-polymc--bin-orange)](https://mpr.makedeb.org/packages/polymc-bin)
[![polymc-git](https://img.shields.io/badge/mpr-polymc--git-orange)](https://mpr.makedeb.org/packages/polymc-git)

```sh
# stable source package:
sudo tap install polymc
# stable binary package:
sudo tap install polymc-bin
# latest git package:
sudo tap install polymc-git
```

### <img src="https://www.vectorlogo.zone/logos/nixos/nixos-icon.svg" height="20" /> Nix

A [Nix derivation](packages/nix/NIX.md) is available.

### <img src="https://www.gentoo.org/assets/img/logo/gentoo-signet.svg" height="20" /> Gentoo

A Gentoo ebuild is available in the [swirl](https://git.swurl.xyz/swirl/ebuilds) overlay, named `games-action/polymc`.

```sh
# as root:
emerge --oneshot eselect-repository
eselect repository enable swirl
emaint sync -r swirl
emerge polymc
# to use latest git version:
sudo tee -a /etc/portage/package.accept_keywords <<< "=games-action/polymc-9999 **"
```

### <img src="https://www.vectorlogo.zone/logos/getfedora/getfedora-icon.svg" height="20"> Fedora

An RPM package is available on [COPR](https://copr.fedorainfracloud.org/coprs/polymc/polymc/).

```sh
sudo dnf copr enable polymc/polymc
sudo dnf install polymc
```

Alternatively, a COPR maintained by a PolyMC user (instead of Jenkins' automated builds) is available [here](https://copr.fedorainfracloud.org/coprs/sentry/polymc).

```sh
sudo dnf copr enable sentry/polymc
sudo dnf install polymc
```

## <img src="https://www.vectorlogo.zone/logos/microsoft/microsoft-icon.svg" height="20" /> Windows

[Windows (32-bit)](https://packages.polymc.org/latest/win32/win32.zip) ([SHA256](https://packages.polymc.org/latest/win32/win32.zip.sha256)) - this is a portable package, you can extract it anywhere and run it. This package needs testing.

## <img src="https://www.vectorlogo.zone/logos/apple/apple-tile.svg" height="20" /> MacOS

MacOS currently does not have any packages. We are still working on setting up MacOS packaging. Meanwhile, you can [build](https://github.com/PolyMC/PolyMC/blob/develop/BUILD.md#macos) it for yourself.

## Development Builds

There are per-commit development builds available [here](https://github.com/PolyMC/PolyMC/actions). These have debug information in the binaries, so their file sizes are relatively larger.
Builds are provided for Linux, AppImage on Linux, Windows, and macOS.

# Help & Support

Feel free to create an issue if you need help. However, you might find it easier to ask in the Discord server.

[![PolyMC Discord](https://img.shields.io/discord/923671181020766230?label=PolyMC%20Discord)](https://discord.gg/xq7fxrgtMP)

For people who don't want to use Discord, we have a Matrix Space which is bridged to the Discord server:

[![PolyMC Space](https://img.shields.io/matrix/polymc:polymc.org?label=PolyMC%20Space&server_fqdn=matrix.polymc.org)](https://matrix.to/#/#polymc:polymc.org)

If there are any issues with the space or you are using a client that does not support the feature here are the individual rooms:

[![Support](https://img.shields.io/matrix/support:polymc.org?label=%23support&server_fqdn=matrix.polymc.org)](https://matrix.to/#/#support:polymc.org)
[![Discussion](https://img.shields.io/matrix/discussion:polymc.org?label=%23discussion&server_fqdn=matrix.polymc.org)](https://matrix.to/#/#discussion:polymc.org)
[![Development](https://img.shields.io/matrix/development:polymc.org?label=%23development&server_fqdn=matrix.polymc.org)](https://matrix.to/#/#development:polymc.org)
[![News](https://img.shields.io/matrix/news:polymc.org?label=%23news&server_fqdn=matrix.polymc.org)](https://matrix.to/#/#news:polymc.org)

# Development

If you want to contribute to PolyMC you might find it useful to join our Discord Server or Matrix Space.

## Building

If you want to build PolyMC yourself, check [BUILD.md](BUILD.md) for build instructions.

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

## Forking/Redistributing/Custom builds policy

Do whatever you want, we don't care. Just follow the license. If you have any questions about this feel free to ask in an issue.
