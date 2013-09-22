## Table of Contents

Build on...

* [Linux](#linux)
* [Windows](#win)
* [OSX](#osx)

## <a id="linux"></a>Linux

```bash
git clone git@github.com:MultiMC/MultiMC5.git # get the code
# if you want a development build also run git checkout develop. otherwise you get a stable build (from master)
cd MultiMC5
```

now you need to install (unless already installed):

1. Qt 5.1.1+ Development tools (http://qt-project.org/downloads)
1. cmake
1. ccmake
1. make
1. g++

once that is done, do these commands:

```bash
mkdir build
cd build
ccmake ..
```

A GUI will pop up. press the c key. now set the build prefix. if you are in /home/username/code/MultiMC5/build then put /home/username/code/MultiMC5/build/run as build prefix. if you want you can choose whatever dir you want, but then you need to adjust the path when running it. to edit the value use the up/down keys to select it and hit return to edit it. after you are done hit return again.  
Also adjust the paths to your qt install.

Then hit c and g

continue with the following commands:

```bash
cmake ..
make
make translations_target # compiles localization files. you may leave this out if your language is english
make install
```
now you compiled it (hupefully) successfully.

to launch it:

```bash
cd run # or whereever its stored
./MultiMC5
```

Congrats. Your MMC5 should run

## <a id="win"></a>Windows

*In work*

## <a id="osx"></a>OSX

*MMC5 is not working on OSX yet. If you get it somehow working anyways, tell us how*
