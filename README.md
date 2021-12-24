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
- [Linux (AMD64) System](https://jenkins.swurl.xyz/job/PolyMC/lastSuccessfulBuild/artifact/lin64-system.tar.zst) - this is a SYSTEM package that is intended to be installed via a package manager. This CAN NOT be used as a portable application!
- [Windows (32-bit)](https://jenkins.swurl.xyz/job/PolyMC/lastSuccessfulBuild/artifact/win32.zip) - this is a PORTABLE package. This package needs testing.
- MacOS currently does not have any packages. We are still working on setting up MacOS packaging.

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
Feel free to create an issue if you need help. However, you should always ask the Matrix room. Be sure to enable spaces first (Settings -> Labs -> Spaces), and then you may join the space:

<a href="https://matrix.to/#/#polymc:polymc.org" alt="PolyMC Matrix Space"><img src="https://img.shields.io/badge/matrix-%23polymc:polymc.org-brightgreen.svg"></a>

If this doesn't work for you, then you may simply join the support room:

<a href="https://matrix.to/#/#support:polymc.org" alt="Matrix Room"><img src="https://img.shields.io/badge/matrix-%23support:polymc.org-brightgreen.svg"></a>

Discord will be made soon.

## Copyright
Copyright 2013-2021 MultiMC contributors
Copyright 2021 PolyMC contributors
