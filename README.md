# Prism Launcher

Prism Launcher is a custom launcher for Minecraft that allows you to easily manage multiple installations of Minecraft at once.

Prism Launcher is a **fork** of PolyMC (and thus a fork of MultiMC) and is not endorsed by either.

We are working on a website and other media, for more info we have a [Discord server](https://discord.gg/hX4g537UNE). Logo and branding also coming soon.

## Installation

- All downloads and instructions for Prism Launcher will soon be available. If you would like to download development builds beforehand, please see [Development Builds](#development-builds).
- Last build status: <https://github.com/PrismLauncher/PrismLauncher/actions>

### Development Builds

There are development builds available [here](https://github.com/PrismLauncher/PrismLauncher/actions). These have debug information in the binaries, so their file sizes are relatively larger.

Portable builds are provided for AppImage on Linux, Windows, and macOS.

## Help & Support

- Join the [Discord Server](https://discord.gg/hX4g537UNE) for now.

## Development

If you would like to contribute to Prism Launcher, please check out the [Issue Page](https://github.com/PrismLauncher/PrismLauncher/issues). You may also find it helpful to join anything listed in [Help & Support](#help--support) for further direction.

## Forking/Redistributing/Custom builds

You are free to make any custom builds or forks as long as you follow the legal responsibilities of the [License](https://github.com/PrismLauncher/PrismLauncher/blob/develop/LICENSE). If your code also makes changes instead of simply repackaging the build, we also suggest that you do the following as a basic courtesy:

- Make clear that your fork is **not** Prism Launcher, nor affiliated with Prism Launcher or any of it's forks thereof.
- Remove any API keys belonging to Prism Launcher in [CMakeLists.txt](https://github.com/PrismLauncher/PrismLauncher/blob/develop/CMakeLists.txt), either by replacing with an empty string (`""`) or creating them yourself.

Keep in mind that by not removing API keys, you are accepting the terms and conditions of the following:

- [Microsoft Identity Platform Terms of Use](https://docs.microsoft.com/en-us/legal/microsoft-identity-platform/terms-of-use)
- [CurseForge 3rd Party API Terms and Conditions](https://support.curseforge.com/en/support/solutions/articles/9000207405-curse-forge-3rd-party-api-terms-and-conditions)

If you do not want to accept these terms, please replcate the associated API keys in the [CMakeLists.txt](https://github.com/PrismLauncher/PrismLauncher/blob/develop/CMakeLists.txt) file with empty strings (`""`).

## License

All launcher code is available under the GPL-3.0-only license.
