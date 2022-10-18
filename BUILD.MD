# Build Instructions

## Contents

- [Getting the source](#getting-the-source)
- [Linux](#linux)
- [Windows](#windows)
- [macOS](#macos)
- [OpenBSD](#openbsd)
- [IDEs and Tooling](#ides-and-tooling)

## Getting the source

Clone the source code using git, and grab all the submodules:

```bash
git clone --recursive https://github.com/PlaceholderMC/PrismLauncher.git # Use git@github.com:PlaceholderMC/PrismLauncher.git if you want to clone the repository with SSH
cd PrismLauncher
```

**The rest of the documentation assumes you have already cloned the repository.**

## Linux

Getting the project to build and run on Linux is easy if you use any modern and up-to-date Linux distribution.

### Build dependencies

- A C++ compiler capable of building C++17 code.
- Qt Development tools 5.12 or newer (`qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools libqt5core5a libqt5network5 libqt5gui5` on Debian-based systems)
  - Alternatively you can also use Qt 6.0 or newer (`qt6-base-dev qtchooser qt6-base-dev-tools libqt6core6 libqt6core5compat6-dev libqt6network6` on Debian (testing/unstable) based systems), if you prefer it.
- cmake 3.15 or newer (`cmake` on Debian-based system)
- extra-cmake-modules (`extra-cmake-modules` on Debian-based system)
- zlib (`zlib1g-dev` on Debian-based system)
- Java JDK (`openjdk-17-jdk` on Debian-based system)
- GL headers (`libgl1-mesa-dev` on Debian-based system)
- scdoc if you want to generate manpages (`scdoc` on Debian-based system)

You can use IDEs, like KDevelop, QtCreator or CLion to open the CMake project, if you want to work on the code.

### Building a portable binary

```bash
cmake -S . -B build \
   -DCMAKE_INSTALL_PREFIX=install
#  -DLauncher_QT_VERSION_MAJOR="6" # if you want to use Qt 6

cmake --build build -j$(nproc)
cmake --install build
cmake --install build --component portable
```

### Building & installing to the system

This is the preferred method of installation, and is suitable for packages.

```bash
cmake -S . -B build \
   -DCMAKE_BUILD_TYPE=Release \
   -DCMAKE_INSTALL_PREFIX="/usr" \ # Use "/usr" when building Linux packages. If building not for package, use "/usr/local"
   -DENABLE_LTO=ON # if you want to enable LTO/IPO
#  -DLauncher_QT_VERSION_MAJOR="6" # if you want to use Qt 6

cmake --build build -j$(nproc)
cmake --install build # Optionally specify DESTDIR for packages (i.e. DESTDIR=${pkgdir} cmake --install ...)
```

> TODO: Create Spec file for RPMs, DEB and Flatpak package

### Building a .deb

Requirements: [makedeb](https://docs.makedeb.org/) installed on your system.

```bash
git clone https://mpr.makedeb.org/polymc.git
cd polymc
makedeb -s
```

The .deb will be located in the directory the repo was cloned in.


### Building an .rpm

Build dependencies are automatically installed using `DNF`, however, you will also need the `rpmdevtools` package (on Fedora),
in order to fetch sources and setup your tree.
You don't need to clone the repo for this; the spec file handles that.

```bash
cd ~
# setup your ~/rpmbuild directory, required for rpmbuild to work.
rpmdev-setuptree
# get the rpm spec file from the polymc-misc repo
wget https://copr-dist-git.fedorainfracloud.org/cgit/sentry/polymc/polymc.git/plain/polymc.spec
# install build dependencies
sudo dnf builddep polymc.spec
# download build sources
spectool -g -R polymc.spec
# now build!
rpmbuild -bb polymc.spec
```

The path to the .rpm packages will be printed once the build is complete.

### Building a Flatpak

You don't need to clone the entire PolyMC repo for this; the Flatpak file handles that.
Both `flatpak` and `flatpak-builder` must be installed on your system to proceed.

```bash
git clone https://github.com/flathub/org.polymc.PolyMC
cd org.polymc.PolyMC
# remove --user --install if you want to build without installing
flatpak-builder --user --install flatbuild org.polymc.PolyMC.yml
```

### Installing Qt using the installer (optional)

1. Run the Qt installer.
2. Choose a place to install Qt.
3. Choose the components that you wish install.

   - You need Qt 5.12.x 64-bit ticked. (or a newer version)
      - Alternatively you can choose Qt 6.0 or newer
   - You need Tools/Qt Creator ticked.
   - Other components are selected by default, you can un-tick them if you don't need them.

4. Accept the license agreements.
5. Double-check the install details and then click "Install".

  - Installation can take a very long time, go grab a cup of tea or something and let it work.

### Loading the project in Qt Creator (optional)

1. Open Qt Creator.
2. Choose `File->Open File or Project`.
3. Navigate to the Launcher source folder you cloned and choose CMakeLists.txt.
4. Read the instructions that just popped up about a build location and choose one.
5. You should see "Run CMake" in the window.

   - Make sure that Generator is set to "Unix Generator (Desktop Qt 5.12.x GCC 64bit)".
      - Alternatively this is probably "Unix Generator (Desktop Qt 6.x.x GCC 64bit)"
   - Hit the "Run CMake" button.
   - You'll see warnings and it might not be clear that it succeeded until you scroll to the bottom of the window.
   - Hit "Finish" if CMake ran successfully.

6. Cross your fingers, and press the "Run" button (bottom left of Qt Creator).

   - If the project builds successfully it will run and the Launcher window will pop up.

**If this doesn't work for you, please let us know on our Discord sever, or Matrix Space.**

## Windows

We recommend using a build workflow based on MSYS2, as it's the easiest way to get all of the build dependencies.

### Dependencies

- [MSYS2](https://www.msys2.org/) - Software Distribution and Building Platform for Windows
  - Make sure to follow all instructions on the webpage.
- [Java JDK 8 or later](https://adoptium.net/)
  - Make sure that "Set JAVA_HOME variable" is enabled in the Adoptium installer.

### Preparing MSYS2

1. Open the *MSYS2 MinGW x86* shortcut from the start menu

   - NOTE: There are multiple different MSYS2 related shortcuts. Make sure you actually opened the right **MinGW** version.
   - We recommend building using the 32-bit distribution of MSYS2, as the 64-bit distribution is known to cause problems with PrismLauncher.

2. Install helpers: Run `pacman -Syu pactoys git` in the MSYS2 shell.
3. Install all build dependencies using `pacboy`: Run `pacboy -S toolchain:p cmake:p ninja:p qt6-base:p qt6-5compat:p qt6-svg:p qt6-imageformats:p quazip-qt6:p extra-cmake-modules:p`.

   - Alternatively you can use Qt 5 (for older Windows versions), by running the following command instead: `pacboy -S toolchain:p cmake:p ninja:p qt5-base:p qt5-svg:p qt5-imageformats:p quazip-qt5:p extra-cmake-modules:p`
   - This might take a while, as it will install Qt and all the build tools required.

### Compile from command line on Windows

1. Open the correct **MSYS2 MinGW x86** shell and clone PrismLauncher by doing `git clone --recursive https://github.com/PlaceholderMC/PrismLauncher.git`, and change directory to the folder you cloned to.
2. Now we can prepare the build itself: Run `cmake -Bbuild -DCMAKE_INSTALL_PREFIX=install -DENABLE_LTO=ON -DLauncher_QT_VERSION_MAJOR=6`. These options will copy the final build to `C:\msys64\home\<your username>\PrismLauncher\install` after the build.

   - NOTE: If you want to build using Qt 5, then remove the `-DLauncher_QT_VERSION_MAJOR=6` parameter

3. Now you need to run the build itself: Run `cmake --build build -jX`, where *X* is the number of cores your CPU has.
4. Now, wait for it to compile. This could take some time, so hopefully it compiles properly.
5. Run the command `cmake --install build`, and it should install PrismLauncher to whatever the `-DCMAKE_INSTALL_PREFIX` was.
6. If you don't want PrismLauncher to store its data in `%APPDATA%`, run `cmake --install build --component portable` after the install process
7. In most cases, whenever compiling, the OpenSSL DLLs aren't put into the directory to where PrismLauncher installs, meaning that you cannot log in. The best way to fix this, is just to do `cp /mingw32/bin/libcrypto-1_1.dll /mingw32/bin/libssl-1_1.dll install`. This should copy the required OpenSSL DLLs to log in.

## macOS

### Install prerequisites

- Install XCode Command Line tools.
- Install the official build of CMake (<https://cmake.org/download/>).
- Install extra-cmake-modules
- Install JDK 8 (<https://adoptium.net/temurin/releases/?variant=openjdk8&jvmVariant=hotspot>).
- Install Qt 5.12 or newer or any version of Qt 6 (recommended)

Using [homebrew](https://brew.sh) you can install these dependencies with a single command:

```bash
brew update # in the case your repositories weren't updated
brew install qt openjdk@17 cmake ninja extra-cmake-modules # use qt@5 if you want to install qt5
```

### XCode Command Line tools

If you don't have XCode Command Line tools installed, you can install them with this command:

```bash
xcode-select --install
```

### Build

Choose an installation path.

This is where the final `PrismLauncher.app` will be constructed when you run `make install`. Supply it as the `CMAKE_INSTALL_PREFIX` argument during CMake configuration. By default, it's in the dist folder, under PrismLauncher.

```bash
mkdir build
cd build
cmake \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_INSTALL_PREFIX:PATH="$(dirname $PWD)/dist/" \
 -DCMAKE_PREFIX_PATH="/path/to/Qt/" \
 -DQt5_DIR="/path/to/Qt/" \
 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.14 \
 -DLauncher_QT_VERSION_MAJOR=6 \ # if you want to use Qt 6
 -DENABLE_LTO=ON \ # if you want to enable LTO/IPO
 -DLauncher_BUILD_PLATFORM=macOS
#-DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" # to build a universal binary (not reccomended for development)
 ..
make install
```

Remember to replace `/path/to/Qt/` with the actual path. For newer Qt installations, it is often in your home directory. (For the Homebrew installation, it's likely to be in `/opt/homebrew/opt/qt`.

**Note:** The final app bundle may not run due to code signing issues, which
need to be fixed with `codesign -fs -`.

## OpenBSD

Tested on OpenBSD 7.0-alpha i386. It should also work on older versions.

### Build dependencies

- A C++ compiler capable of building C++11 code (included in base system).
- Qt Development tools 5.12 or newer ([meta/qt5](https://openports.se/meta/qt5)).
- cmake 3.1 or newer ([devel/cmake](https://openports.se/devel/cmake)).
- extra-cmake-modules ([devel/kf5/extra-cmake-modules](https://openports.se/devel/kf5/extra-cmake-modules))
- zlib (included in base system).
- Java JDK ([devel/jdk-1.8](https://openports.se/devel/jdk/1.8)).
- GL headers (included in base system).
- lwjgl ([games/lwjgl](https://openports.se/games/lwjgl) and [games/lwjgl3](https://openports.se/games/lwjgl3)).

You can use IDEs, like KDevelop or QtCreator, to open the CMake project if you want to work on the code.

### Building a portable binary

```bash
mkdir install
# configure the project
cmake -S . -B build \
   -DCMAKE_INSTALL_PREFIX=./install -DCMAKE_PREFIX_PATH=/usr/local/lib/qt5/cmake -DENABLE_LTO=ON
# build
cd build
make -j$(nproc) install
cmake --install install --component portable
```

### Building & installing to the system

This is the preferred method of installation, and is suitable for packages.

```bash
# configure everything
cmake -S . -B build \
   -DCMAKE_BUILD_TYPE=Release \
   -DCMAKE_INSTALL_PREFIX="/usr/local" \ # /usr/local is default in OpenBSD and FreeBSD
   -DCMAKE_PREFIX_PATH=/usr/local/lib/qt5/cmake # use linux layout and point to qt5 libs
   -DENABLE_LTO=ON # if you want to enable LTO/IPO
cd build
make -j$(nproc) install # Optionally specify DESTDIR for packages (i.e. DESTDIR=${pkgdir})
```

## IDEs and Tooling

There are a few tools that you can set up to make your development workflow smoother. In addition, some IDEs also require a bit more setup to work with Qt and CMake.

### ccache

**ccache** is a compiler cache. It speeds up recompilation by caching previous compilations and detecting when the same compilation is being done again.

You can [download it here](https://ccache.dev/download.html). After setting up, builds will be incremental, and the builds after the first one will be much faster.

### VS Code

To set up VS Code, you can download [the C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools), since it provides IntelliSense auto complete, linting, formatting, and various other features.

Then, you need to setup the configuration. Go into the command palette and open up C/C++: Edit Configurations (UI). There, add a new configuration for PrismLauncher.

1. Add the path to your Qt `include` folder to `includePath`
2. Add `-L/{path to your Qt installation}/lib` to `compilerArgs`
3. Set `compileCommands` to `${workspaceFolder}/build/compile_commands.json`
4. Set `cppStandard` to `c++14` or higher.

For step 3 to work, you also have to reconfigure CMake to generate a `compile_commands.json` file. To do this, add `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` to the end of your CMake configuration command and run it again. You should see a file at `build/compile_commands.json`.

Now the VS Code setup should be fully working. To test, open up some files and see if any error squiggles appear. If there are none, it's working properly!

Here is an example of what `.vscode/c_cpp_properties.json` looks like on macOS with Qt installed via Homebrew:

```json
{
    "configurations": [
        {
            "name": "Mac (PrismLauncher)",
            "includePath": [
                "${workspaceFolder}/**",
                "/opt/homebrew/opt/qt@5/include/**"
            ],
            "defines": [],
            "macFrameworkPath": [
                "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks"
            ],
            "compilerPath": "/usr/bin/clang",
            "compilerArgs": [
                "-L/opt/homebrew/opt/qt@5/lib"
            ],
            "compileCommands": "${workspaceFolder}/build/compile_commands.json",
            "cStandard": "c17",
            "cppStandard": "c++14",
            "intelliSenseMode": "macos-clang-arm64"
        }
    ],
    "version": 4
}
```

### CLion

1. Open CLion
2. Choose `File->Open`
3. Navigate to the source folder
4. Go to settings `Ctrl+Alt+S`
5. Navigate to `Toolchains` in `Build, Execution, Deployment`
   - Set the correct build tools ([see here](https://i.imgur.com/daFAdVe.png))
   - CMake: `cmake` (optional)
   - Make: `make` (optional)
   - C Compiler: `gcc`
   - C++ Compiler: `g++`
   - Debugger: `gdb` (optional)
6. Navigate to `CMake` in `Build, Execution, Deployment`
   - Set `Build directory` to `build`
7. Navigate to `Edit Configurations`  ([see here](https://i.imgur.com/fu53nc3.png))
   - Create a new configuration
   - Name: `All`
   - Target: `All targets`
   - Choose the newly added configuration as default

Now you should be able to build and test PrismLauncher with the `Build` and `Run` buttons.
