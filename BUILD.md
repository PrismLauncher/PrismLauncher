Build Instructions
==================

# Contents

* [Note](#note)
* [Getting the source](#source)
* [Linux](#linux)
* [Windows](#windows)
* [macOS](#macos)

# Note

MultiMC is a portable application and is not supposed to be installed into any system folders.
That would be anything outside your home folder. Before running `make install`, make sure
you set the install path to something you have write access to. Never build this under
an administrator/root level account. Don't use `sudo`. It won't work and it's not supposed to work.
Also note that this guide is for development purposes only.  
**No support is given for building your own fork or special build for any reason whatsoever**.

# Branding, identifying marks and API keys

The logo and related assets are All Rights Reserved and may only be used in official builds of MultiMC hosted on multimc.org, and as such, are not, and will not be included in this repository. The source is only provided for the purpose of collaboration. 

API keys are necessary for Microsoft account functionality. More info in [(Not) Secrets](https://github.com/MultiMC/Launcher/tree/develop/notsecrets)

# Getting the source

Clone the source code using git and grab all the submodules:

```
git clone https://github.com/MultiMC/Launcher.git
git submodule init
git submodule update
```

# Linux

Getting the project to build and run on Linux is easy if you use any modern and up-to-date linux distribution.

## Build dependencies
* A C++ compiler capable of building C++11 code.
* Qt 5.6+ Development tools (http://qt-project.org/downloads) ("Qt Online Installer for Linux (64 bit)") or the equivalent from your package manager. It is always better to use the Qt from your distribution, as long as it has a new enough version.
* cmake 3.1 or newer
* zlib (for example, `zlib1g-dev`)
* Java JDK 8 (for example, `openjdk-8-jdk`)
* GL headers (for example, `libgl1-mesa-dev`)

### Building from command line
You need a source folder, a build folder and an install folder.

Let's say you want everything in `~/MultiMC/`:

```
# make all the folders
mkdir ~/MultiMC && cd ~/MultiMC
mkdir build
mkdir install
# clone the complete source
git clone --recursive https://github.com/MultiMC/Launcher.git src
# configure the project
cd build
cmake -DCMAKE_INSTALL_PREFIX=../install ../src
# build & install (use -j with the number of cores your CPU has)
make -j8 install
```

You can use IDEs like KDevelop or QtCreator to open the CMake project if you want to work on the code.

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

**If this doesn't work for you, let us know on IRC ([Esper/#MultiMC](http://webchat.esper.net/?nick=&channels=MultiMC))!**

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

**These build instructions worked for me (Drayshak) on a fresh Windows 8 x64 Professional install. If they don't work for you, let us know on IRC ([Esper/#MultiMC](http://webchat.esper.net/?nick=&channels=MultiMC))!**
### Compile from command line on Windows
1. If you installed Qt with the web installer, there should be a shortcut called `Qt 5.4 for Desktop (MinGW 4.9 32-bit)` in the Start menu on Windows 7 and 10. Best way to find it is to search for it. Do note you cannot just use cmd.exe, you have to use the shortcut, otherwise the proper MinGW software will not be on the PATH.
2. Once that is open, change into your user directory, and clone MultiMC by doing `git clone --recursive https://github.com/MultiMC/Launcher.git`, and change directory to the folder you cloned to.
3. Make a build directory, and change directory to the directory and do `cmake -G "MinGW Makefiles" -DCMAKE_INSTALL_PREFIX=C:\Path\that\makes\sense\for\you`. By default, it will install to C:\Program Files (x86), which you might not want, if you want a local installation. If you want to install it to that directory, make sure to run the command window as administrator.
3. Do `mingw32-make -jX`, where X is the number of cores your CPU has plus one.
4. Now to wait for it to compile. This could take some time. Hopefully it compiles properly.
5. Run the command `mingw32-make install`, and it should install MultiMC, to whatever the `-DCMAKE_INSTALL_PREFIX` was.
6. In most cases, whenever compiling, the OpenSSL dll's aren't put into the directory to where MultiMC installs, meaning you cannot log in. The best way to fix this is just to do `copy C:\OpenSSL-Win32\*.dll C:\Where\you\installed\MultiMC\to`. This should copy the required OpenSSL dll's to log in.

# macOS

### Install prerequisites:
- Install XCode Command Line tools
- Install the official build of CMake (https://cmake.org/download/)
- Install JDK 8 (https://www.oracle.com/java/technologies/javase/javase-jdk8-downloads.html)
- Get Qt 5.6 and install it (https://download.qt.io/new_archive/qt/5.6/5.6.3/)

### XCode Command Line tools

If you don't have XCode CommandLine tools installed, you can install them by using this command in the Terminal App

```bash
xcode-select --install
```

### Build

Pick an installation path - this is where the final `.app` will be constructed when you run `make install`. Supply it as the `CMAKE_INSTALL_PREFIX` argument during CMake configuration.

```
git clone --recursive https://github.com/MultiMC/Launcher.git
cd Launcher
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
