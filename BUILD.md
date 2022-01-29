Build Instructions
==================

# Contents

* [Note](#note)
* [Getting the source](#source)
* [Linux](#linux)
* [Windows](#windows)
* [macOS](#macos)


# Getting the source

Clone the source code using git and grab all the submodules:

```
git clone https://github.com/PolyMC/PolyMC.git
git submodule init
git submodule update
```

# Linux

Getting the project to build and run on Linux is easy if you use any modern and up-to-date linux distribution.

## Build dependencies
* A C++ compiler capable of building C++11 code.
* Qt Development tools 5.6 or newer  (`qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools libqt5core5a libqt5network5 libqt5gui5` on Debian-based system)
* cmake 3.1 or newer (`cmake` on Debian-based system)
* zlib (`zlib1g-dev` on Debian-based system)
* Java JDK (`openjdk-17-jdk`on Debian-based system)
* GL headers (`libgl1-mesa-dev` on Debian-based system)

### Building a .deb

You need to install the build dependencies first

```
git clone https://github.com/PolyMC/PolyMC.git
git submodule init && git submodule update
cd packages/debian
./makedeb.sh
```

If everything works correctly, the .deb will be next to the build script, in `PolyMC/packages/debian`

### Building a .rpm

Build dependencies are automatically installed using `dnf`, but you do need the `rpmdevtools` package (on Fedora)
in order to fetch sources and setup your tree.

```
cd ~
# setup your ~/rpmbuild directory, required for rpmbuild to work.
rpmdev-setuptree
# note: submodules are not needed here, as the spec file will download the tarball instead
git clone https://github.com/PolyMC/PolyMC.git
cd PolyMC/packages/rpm
# install build dependencies
sudo dnf builddep polymc.spec
# download build sources
spectool -g -R polymc.spec
# now build!
rpmbuild -bb polymc.spec
```

The path to the rpm packages will be printed when the build is complete.

### Building from command line
You need a source folder, a build folder and an install folder.

```
# make all the folders
mkdir ~/PolyMC && cd ~/PolyMC
mkdir build
mkdir install
# clone the complete source
git clone --recursive https://github.com/PolyMC/PolyMC.git src
# configure the project
cd build
cmake -DCMAKE_INSTALL_PREFIX=../install ../src
make -j$(nproc) install
```

You can use IDEs like KDevelop or QtCreator to open the CMake project if you want to work on the code.

### Building & Installing to the System
This is the preferred method for installation, and is suitable for packages.

```
git clone --recursive https://github.com/PolyMC/PolyMC.git && cd PolyMC

# configure everything
cmake -S . -B build \
	-DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="/usr" \ # Use "/usr" for packages, otherwise, leave it at the default "/usr/local".
    -DLauncher_LAYOUT=lin-system

make -j$(nproc) install # Optionally specify DESTDIR for packages (i.e. DESTDIR=${pkgdir})
```

### Installing Qt using the installer (optional)
1. Run the Qt installer.
2. Choose a place to install Qt.
3. Choose the components you want to install.
    - You need Qt 5.6.x 64-bit ticked.
    - You need Tools/Qt Creator ticked.
    - Other components are selected by default, you can untick them if you don't need them.
4. Accept the license agreements.
5. Double check the install details and then click "Install".
    - Installation can take a very long time, go grab a cup of tea or something and let it work.

### Loading the project in Qt Creator (optional)
1. Open Qt Creator.
2. Choose `File->Open File or Project`.
3. Navigate to the Launcher source folder you cloned and choose CMakeLists.txt.
4. Read the instructions that just popped up about a build location and choose one.
5. You should see "Run CMake" in the window.
    - Make sure that Generator is set to "Unix Generator (Desktop Qt 5.6.x GCC 64bit)".
    - Hit the "Run CMake" button.
    - You'll see warnings and it might not be clear that it succeeded until you scroll to the bottom of the window.
    - Hit "Finish" if CMake ran successfully.
6. Cross your fingers and press the Run button (bottom left of Qt Creator).
    - If the project builds successfully it will run and the Launcher window will pop up.

**If this doesn't work for you, let us know on our Discord.**

# Windows

Getting the project to build and run on Windows is easy if you use Qt's IDE, Qt Creator. The project will simply not compile using Microsoft build tools, because that's not something we do. If it does compile, it is by chance only.

## Dependencies
* [Qt 5.6+ Development tools](http://qt-project.org/downloads) -- Qt Online Installer for Windows
    - http://download.qt.io/new_archive/qt/5.6/5.6.0/qt-opensource-windows-x86-mingw492-5.6.0.exe
    - Download the MinGW version (MSVC version does not work).
* [OpenSSL](https://github.com/IndySockets/OpenSSL-Binaries/tree/master/Archive/) -- Win32 OpenSSL, version 1.0.2g (from 2016)
    - https://github.com/IndySockets/OpenSSL-Binaries/raw/master/Archive/openssl-1.0.2g-i386-win32.zip
    - the usual OpenSSL for Windows (http://slproweb.com/products/Win32OpenSSL.html) only provides the newest version of OpenSSL, and we need the 1.0.2g version
    - **Download the 32-bit version, not 64-bit.**
    - Microsoft Visual C++ 2008 Redist is required for this, there's a link on the OpenSSL download page above next to the main download.
    - We use a custom build of OpenSSL that doesn't have this dependency. For normal development, the custom build is not necessary though.
* [zlib 1.2+](http://gnuwin32.sourceforge.net/packages/zlib.htm) - the Setup is fine
* [Java JDK 8](https://adoptium.net/releases.html?variant=openjdk8) - Use the MSI installer.
* [CMake](http://www.cmake.org/cmake/resources/software.html) -- Windows (Win32 Installer)

Ensure that OpenSSL, zlib, Java and CMake are on `PATH`.

## Getting set up

### Installing Qt
1. Run the Qt installer
2. Choose a place to install Qt (C:\Qt is the default),
3. Choose the components you want to install
    - You need Qt 5.6 (32 bit) ticked,
    - You need Tools/Qt Creator ticked,
    - Other components are selected by default, you can untick them if you don't need them.
4. Accept the license agreements,
5. Double check the install details and then click "Install"
    - Installation can take a very long time, go grab a cup of tea or something and let it work.

### Installing OpenSSL
1. Download .zip file from the link above.
2. Unzip and add the directory to PATH, so CMake can find it.

### Installing CMake
1. Run the CMake installer,
2. It's easiest if you choose to add CMake to the PATH for all users,
    - If you don't choose to do this, remember where you installed CMake.

### Loading the project
1. Open Qt Creator,
2. Choose File->Open File or Project,
3. Navigate to the Launcher source folder you cloned and choose CMakeLists.txt,
4. Read the instructions that just popped up about a build location and choose one,
5. If you chose not to add CMake to the system PATH, tell Qt Creator where you installed it,
    - Otherwise you can skip this step.
6. You should see "Run CMake" in the window,
    - Make sure that Generator is set to "MinGW Generator (Desktop Qt 5.6.x MinGW 32bit)",
    - Hit the "Run CMake" button,
    - You'll see warnings and it might not be clear that it succeeded until you scroll to the bottom of the window.
    - Hit "Finish" if CMake ran successfully.
7. Cross your fingers and press the Run button (bottom left of Qt Creator)!
    - If the project builds successfully it will run and the Launcher window will pop up,
    - Test OpenSSL by making an instance and trying to log in. If Qt Creator couldn't find OpenSSL during the CMake stage, login will fail and you'll get an error.

The following .dlls are needed for the app to run (copy them to build directory if you want to be able to move the build to another pc):
```
platforms/qwindows.dll
libeay32.dll
libgcc_s_dw2-1.dll
libssp-0.dll
libstdc++-6.dll
libwinpthread-1.dll
Qt5Core.dll
Qt5Gui.dll
Qt5Network.dll
Qt5Svg.dll
Qt5Widgets.dll
Qt5Xml.dll
ssleay32.dll
zlib1.dll
```

**These build instructions worked for me (Drayshak) on a fresh Windows 8 x64 Professional install. If they don't work for you, let us know on our Discord.**
### Compile from command line on Windows
1. If you installed Qt with the web installer, there should be a shortcut called `Qt 5.4 for Desktop (MinGW 4.9 32-bit)` in the Start menu on Windows 7 and 10. Best way to find it is to search for it. Do note you cannot just use cmd.exe, you have to use the shortcut, otherwise the proper MinGW software will not be on the PATH.
2. Once that is open, change into your user directory, and clone PolyMC by doing `git clone --recursive https://github.com/PolyMC/PolyMC.git`, and change directory to the folder you cloned to.
3. Make a build directory, and change directory to the directory and do `cmake -G "MinGW Makefiles" -DCMAKE_INSTALL_PREFIX=C:\Path\that\makes\sense\for\you`. By default, it will install to C:\Program Files (x86), which you might not want, if you want a local installation. If you want to install it to that directory, make sure to run the command window as administrator.
3. Do `mingw32-make -jX`, where X is the number of cores your CPU has plus one.
4. Now to wait for it to compile. This could take some time. Hopefully it compiles properly.
5. Run the command `mingw32-make install`, and it should install PolyMC, to whatever the `-DCMAKE_INSTALL_PREFIX` was.
6. In most cases, whenever compiling, the OpenSSL dll's aren't put into the directory to where PolyMC installs, meaning you cannot log in. The best way to fix this is just to do `copy C:\OpenSSL-Win32\*.dll C:\Where\you\installed\PolyMC\to`. This should copy the required OpenSSL dll's to log in.

# macOS

### Install prerequisites:
- Install XCode Command Line tools
- Install the official build of CMake (https://cmake.org/download/)
- Install JDK 8 (https://adoptium.net/releases.html?variant=openjdk8&jvmVariant=hotspot)
- Get Qt 5.6 and install it (https://download.qt.io/new_archive/qt/5.6/5.6.3/)

### XCode Command Line tools

If you don't have XCode CommandLine tools installed, you can install them by using this command in the Terminal App

```bash
xcode-select --install
```

### Build

Pick an installation path - this is where the final `PolyMC.app` will be constructed when you run `make install`. Supply it as the `CMAKE_INSTALL_PREFIX` argument during CMake configuration. By default, it's in the dist folder under PolyMC

```
git clone --recursive https://github.com/PolyMC/PolyMC.git
cd PolyMC
mkdir build
cd build
cmake \
 -DCMAKE_C_COMPILER=/usr/bin/clang \
 -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_INSTALL_PREFIX:PATH="$(dirname $PWD)/dist/" \
 -DCMAKE_PREFIX_PATH="/path/to/Qt5.6/" \
 -DQt5_DIR="/path/to/Qt5.6/" \
 -DLauncher_LAYOUT=mac-bundle \
 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.7 \
 ..
make install
```

**Note:** The final app bundle may not run due to code signing issues, which
need to be fixed with `codesign -fs -`.
