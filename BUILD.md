Build Instructions
==================

# Contents
* [Linux](#linux)
* [Windows](#windows)
* [OS X](#os-x)

# Linux

Getting the project to build and run on Linux is easy if you use Ubuntu 13.10 (or 13.04) and Qt's IDE, Qt Creator.

## Dependencies
* Qt 5.1.1+ Development tools (http://qt-project.org/downloads) ("Qt Online Installer for Linux (64 bit)")
* A copy of the MultiMC source (clone it with git)
* cmake
* build-essentials
* zlib (for example, zlib1g-dev)
* java (for example, openjdk-7-jdk)
* GL headers (for example, libgl1-mesa-dev)

## Getting set up

### Installing dependencies
Just run `sudo apt-get install <dependency>` for each dependency (other than Qt and the MultiMC source) from above.

### Installing Qt
1. Run the Qt installer
2. Choose a place to install Qt,
3. Choose the components you want to install
    - You need Qt 5.1.1/gcc 64-bit ticked,
    - You need Tools/Qt Creator ticked,
    - Other components are selected by default, you can untick them if you don't need them.
4. Accept the license agreements,
5. Double check the install details and then click "Install"
    - Installation can take a very long time, go grab a cup of tea or something and let it work.

### Loading the project
1. Open Qt Creator,
2. Choose File->Open File or Project,
3. Navigate to the MultiMC5 source folder you cloned and choose CMakeLists.txt,
4. Read the instructions that just popped up about a build location and choose one,
5. You should see "Run CMake" in the window,
    - Make sure that Generator is set to "Unix Generator (Desktop Qt 5.1.1 GCC 64bit)",
    - Hit the "Run CMake" button,
    - You'll see warnings and it might not be clear that it succeeded until you scroll to the bottom of the window.
    - Hit "Finish" if CMake ran successfully.
6. Cross your fingers and press the Run button (bottom left of Qt Creator)!
    - If the project builds successfully it will run and the MultiMC5 window will pop up.

*These build instructions worked for me (Drayshak) on a fresh Ubuntu 13.10 x64 install. If they don't work for you, let us know on IRC (Esper/#MultiMC)!*

# Windows

Getting the project to build and run on Windows is easy if you use Qt's IDE, Qt Creator. The project will simply not compile using VC's build tools as it uses some C++11 features that aren't implemented in it at the time of writing.

## Dependencies
* Qt 5.1.1+ Development tools (http://qt-project.org/downloads) ("Qt Online Installer for Windows")
* OpenSSL (http://slproweb.com/products/Win32OpenSSL.html) ("Win32 OpenSSL v1.0.1e Light")
    - Microsoft Visual C++ 2008 Redist. is required for this, there's a link on the OpenSSL download page above next to the main download.
* CMake (http://www.cmake.org/cmake/resources/software.html) ("Windows (Win32 Installer)")
* A copy of the MultiMC source (clone it with git)

## Getting set up

### Installing Qt
1. Run the Qt installer
2. Choose a place to install Qt (C:\Qt is the default),
3. Choose the components you want to install
    - You need Qt 5.1.1/MinGW 4.8 (32 bit) ticked,
    - You need Tools/Qt Creator ticked,
    - Other components are selected by default, you can untick them if you don't need them.
4. Accept the license agreements,
5. Double check the install details and then click "Install"
    - Installation can take a very long time, go grab a cup of tea or something and let it work.

### Installing OpenSSL
1. Run the OpenSSL installer,
2. It's best to choose the option to copy OpenSSL DLLs to the /bin directory
    - If you do this you'll need to add that directory (the default being C:\OpenSSL-Win32\bin) to your PATH system variable (Google how to do this, or use this guide for Java: http://www.java.com/en/download/help/path.xml).

### Installing CMake
1. Run the CMake installer,
2. It's easiest if you choose to add CMake to the PATH for all users,
    - If you don't choose to do this, remember where you installed CMake.

### Loading the project
1. Open Qt Creator,
2. Choose File->Open File or Project,
3. Navigate to the MultiMC5 source folder you cloned and choose CMakeLists.txt,
4. Read the instructions that just popped up about a build location and choose one,
5. If you chose not to add CMake to the system PATH, tell Qt Creator where you installed it,
    - Otherwise you can skip this step.
6. You should see "Run CMake" in the window,
    - Make sure that Generator is set to "MinGW Generator (Desktop Qt 5.1.1 MinGW 32bit)",
    - Hit the "Run CMake" button,
    - You'll see warnings and it might not be clear that it succeeded until you scroll to the bottom of the window.
    - Hit "Finish" if CMake ran successfully.
7. Cross your fingers and press the Run button (bottom left of Qt Creator)!
    - If the project builds successfully it will run and the MultiMC5 window will pop up,
    - Test OpenSSL by making an instance and trying to log in. If Qt Creator couldn't find OpenSSL during the CMake stage, login will fail and you'll get an error.

*These build instructions worked for me (Drayshak) on a fresh Windows 8 x64 Professional install. If they don't work for you, let us know on IRC (Esper/#MultiMC)!*

# OS X

### Install prerequisites:
1. install homebrew
2. brew install qt5
3. brew tap homebrew/versions
4. brew install gcc48
5. brew install cmake

### Build
1. git clone https://github.com/MultiMC/MultiMC5.git
2. cd MultiMC5
3. mkdir build
4. cd build
5. export CMAKE_PREFIX_PATH=/usr/local/opt/qt5
6. export CC=/usr/local/bin/gcc-4.8
7. export CXX=/usr/local/bin/g++-4.8
8. cmake ..
9. make
  
*These build instructions were taken and adapted from https://gist.github.com/number5/7250865 If they don't work for you, let us know on IRC (Esper/#MultiMC)!*
