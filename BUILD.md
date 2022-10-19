# Build Instructions

Full build instructions will be available on [the website](https://prismlauncher.org/wiki/development/build-instructions/).

If you would like to contribute or fix an issue with the Build instructions you will be able to do so [here](https://github.com/PrismLauncher/website/blob/master/src/wiki/development/build-instructions.md).

## Getting the source

Clone the source code using git, and grab all the submodules. This is generic for all platforms you want to build on.
```
git clone --recursive https://github.com/PrismLauncher/PrismLauncher
cd PrismLauncher
```

## Linux

This guide will mostly mention dependant packages by their Debian naming and commands are done by a user in the sudoers file.
### Dependencies

- A C++ compiler capable of building C++17 code (can be found in the package `build-essential`).
- Qt Development tools 5.12 or newer (on Debian 11 or Debian-based distributions, `qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools libqt5core5a libqt5network5 libqt5gui5`).
- `cmake` 3.15 or newer.
- `extra-cmake-modules`.
- zlib (`zlib1g-dev` on Debian 11 or Debian-based distributions).
- Java Development Kit (Java JDK) (`openjdk-17-jdk` on Debian 11 or Debian-based distributions).
- Mesa GL headers (`libgl1-mesa-dev` on Debian 11 or Debian-based distributions).
- (Optional) `scdoc` to generate man pages.

In conclusion, to check if all you need is installed (including optional):

```
sudo apt install build-essential qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools libqt5core5a libqt5network5 libqt5gui5 cmake extra-cmake-modules zlib1g-dev openjdk-17-jdk libgl1-mesa-dev scdoc
```

### Compiling
#### Building and installing on the system
This is usually the suggested way to build the client.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="/usr" -DENABLE_LTO=ON
cmake --build build -j$(nproc)
sudo cmake --install build
```

#### Building a portable binary

```
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=install
cmake --build build -j$(nproc)
cmake --install build
cmake --install build --component portable
```

## macOS

### Prerequisites
To build Prism on macOS, you will need to do the following:
- Install Xcode Command Line tools
- Install the official build of CMake (https://cmake.org/download/)
- Install extra-cmake-modules
- Install JDK 17 (for example, [Temurin Open JDK](https://adoptium.net/temurin/releases/?version=17))
- Install Qt 5.12 or newer, or any version of Qt6 (recommended)
  - If you're installing through an official [Qt Online Installer](https://www.qt.io/download), selecting `macOS` component in `Qt > Qt<version> > macOS` should be enough.
  - Also, if you're building on Qt6, you will need to install Qt5 Compatibility Module (which is located in `Qt > Qt6.x.x > Qt5 Compatibility Module`).
  - If you're installing Qt via Homebrew, no extra steps are necessary, as Homebrew builds of Qt already contain all the necessary libraries.

If you're using Homebrew, you can install all the dependencies above by using this command:
If you want to build with Qt6:
```sh
brew update && brew install qt openjdk@17 cmake ninja extra-cmake-modules
```
Or if you want to build with Qt5:
```sh
brew update && brew install qt@5 openjdk@17 cmake ninja extra-cmake-modules
```

#### Xcode Command Line Tools
You can install Xcode Command Line Tools via the following command
```sh
xcode-select --install
```
Additionally, if xcode-select fails to detect the correct version of Xcode and installs outdated version of CLT, you can download the recent CLT manually from [official Apple Developer website](https://developer.apple.com/download/all/?q=xcode%20command%20line%20tools), for the reference you can use [Xcode Releases website](https://xcodereleases.com/).

There is no need for the full Xcode installation, Command Line tools should be enough.
### Building
Choose an installation path.
This is where the final `PrismLauncher.app` will be constructed when you run `make install`. Supply it as the `CMAKE_INSTALL_PREFIX` argument during CMake configuration. By default, it's in the dist folder, under PrismLauncher (the `$PWD/dist/` value).

Run the `cmake` command. Choose the one of templates below carefully.

Also remember to change `/Path/To/Qt` to your actual Qt installation path (Qt Online installer by default installs to `~/Qt/6.x.x` for Qt 6 or `~/Qt/5.15.x/clang_64` for Qt5; Homebrew should install Qt to `/usr/local/opt/qt@5` or `/usr/local/opt/qt@6`)

If you're using Qt 6:
```sh
QT_PATH="/Path/To/Qt"
cmake \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_INSTALL_PREFIX:PATH="$PWD/dist/" \
 -DCMAKE_PREFIX_PATH="$QT_PATH" \
 -DQt6_DIR="$QT_PATH" \
 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.14 \
 -DLauncher_QT_VERSION_MAJOR=6 \
 -DENABLE_LTO=ON \ # if you want to enable LTO/IPO
 -DLauncher_BUILD_PLATFORM=macOS
```
If you're using Qt 5:
```sh
QT_PATH="/Path/To/Qt"
cmake -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_INSTALL_PREFIX:PATH="$PWD/dist/" \
 -DCMAKE_PREFIX_PATH="$QT_PATH" \
 -DQt5_DIR="$QT_PATH" \
 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13 \
 -DLauncher_QT_VERSION_MAJOR=5 \
 -DENABLE_LTO=ON \ # if you want to enable LTO/IPO
 -DLauncher_BUILD_PLATFORM=macOS
 ```
P.S. There seems to be a bug in Homebrew's builds of Qt6, which throws an error, stating that `'path' is unavailable: introduced in macOS 10.15`. If you will encounter this error, you can either install Qt6 via official Qt6 Online Installer, reconfigure the build by running cmake again, this time changing `CMAKE_OSX_DEPLOYMENT_TARGET` to `10.15` or higher, or simply use Qt5.


When you will see the message about the successful configuring, run the following to build PrismLauncher:
```
make install
```

After the build (can take a few minutes), you will find `PrismLauncher.app` in the folder that you've specified in `CMAKE_INSTALL_PREFIX` (by default this is `dist/` subfolder).

**Note:** The final app bundle may not run due to code signing issues, which
need to be fixed with `codesign -fs -`
