# Third-party libraries

This folder has third-party or otherwise external libraries needed for other parts to work.

## classparser
A simplistic parser for Java class files.

This library has served as a base for some (much more full-featured and advanced) work under NDA for AVG. It, however, should NOT be confused with that work.

Copyright belongs to Petr Mrázek, unless explicitly stated otherwise in the source files. Available under the Apache 2.0 license.

## hoedown
Hoedown is a revived fork of Sundown, the Markdown parser based on the original code of the Upskirt library by Natacha Porté.

See [github repo](https://github.com/hoedown/hoedown).

## iconfix
This was originally part of the razor-qt project and the Qt toolkit, respecitvely. Its sole purpose is to reimplement Qt's icon loading logic to prevent it from using any platform plugins that could break icon loading.

Licensed under LGPL 2.1

## javacheck
Simple Java tool that prints the JVM details - version and platform bitness.

Do what you want with it. It is so trivial that noone cares.

## launcher
Java launcher part for Minecraft.

It:
* Starts a process
* Waits for a launch script on stdin
* Consumes the launch script you feed it
* Proceeds with launch when it gets the `launcher` command

This means the process is essentially idle until the final command is sent. You can, for example, attach a profiler before you send it.

A `legacy` and `onesix` launchers are available.

* `legacy` is intended for use with Minecraft versions < 1.6 and is deprecated.
* `onesix` can handle launching any Minecraft version, at the cost of some extra features `legacy` enables (custom window icon and title).

Example (some parts have been censored):
```
mod legacyjavafixer-1.0
mainClass net.minecraft.launchwrapper.Launch
param --username
param CENSORED
param --version
param MultiMC5
param --gameDir
param /home/peterix/minecraft/FTB/17ForgeTest/minecraft
param --assetsDir
param /home/peterix/minecraft/mmc5/assets
param --assetIndex
param 1.7.10
param --uuid
param CENSORED
param --accessToken
param CENSORED
param --userProperties
param {}
param --userType
param mojang
param --tweakClass
param cpw.mods.fml.common.launcher.FMLTweaker
windowTitle MultiMC: 172ForgeTest
windowParams 854x480
userName CENSORED
sessionId token:CENSORED:CENSORED
cp /home/peterix/minecraft/FTB/libraries/com/mojang/realms/1.3.5/realms-1.3.5.jar
cp /home/peterix/minecraft/FTB/libraries/org/apache/commons/commons-compress/1.8.1/commons-compress-1.8.1.jar
cp /home/peterix/minecraft/FTB/libraries/org/apache/httpcomponents/httpclient/4.3.3/httpclient-4.3.3.jar
cp /home/peterix/minecraft/FTB/libraries/commons-logging/commons-logging/1.1.3/commons-logging-1.1.3.jar
cp /home/peterix/minecraft/FTB/libraries/org/apache/httpcomponents/httpcore/4.3.2/httpcore-4.3.2.jar
cp /home/peterix/minecraft/FTB/libraries/java3d/vecmath/1.3.1/vecmath-1.3.1.jar
cp /home/peterix/minecraft/FTB/libraries/net/sf/trove4j/trove4j/3.0.3/trove4j-3.0.3.jar
cp /home/peterix/minecraft/FTB/libraries/com/ibm/icu/icu4j-core-mojang/51.2/icu4j-core-mojang-51.2.jar
cp /home/peterix/minecraft/FTB/libraries/net/sf/jopt-simple/jopt-simple/4.5/jopt-simple-4.5.jar
cp /home/peterix/minecraft/FTB/libraries/com/paulscode/codecjorbis/20101023/codecjorbis-20101023.jar
cp /home/peterix/minecraft/FTB/libraries/com/paulscode/codecwav/20101023/codecwav-20101023.jar
cp /home/peterix/minecraft/FTB/libraries/com/paulscode/libraryjavasound/20101123/libraryjavasound-20101123.jar
cp /home/peterix/minecraft/FTB/libraries/com/paulscode/librarylwjglopenal/20100824/librarylwjglopenal-20100824.jar
cp /home/peterix/minecraft/FTB/libraries/com/paulscode/soundsystem/20120107/soundsystem-20120107.jar
cp /home/peterix/minecraft/FTB/libraries/io/netty/netty-all/4.0.10.Final/netty-all-4.0.10.Final.jar
cp /home/peterix/minecraft/FTB/libraries/com/google/guava/guava/16.0/guava-16.0.jar
cp /home/peterix/minecraft/FTB/libraries/org/apache/commons/commons-lang3/3.2.1/commons-lang3-3.2.1.jar
cp /home/peterix/minecraft/FTB/libraries/commons-io/commons-io/2.4/commons-io-2.4.jar
cp /home/peterix/minecraft/FTB/libraries/commons-codec/commons-codec/1.9/commons-codec-1.9.jar
cp /home/peterix/minecraft/FTB/libraries/net/java/jinput/jinput/2.0.5/jinput-2.0.5.jar
cp /home/peterix/minecraft/FTB/libraries/net/java/jutils/jutils/1.0.0/jutils-1.0.0.jar
cp /home/peterix/minecraft/FTB/libraries/com/google/code/gson/gson/2.2.4/gson-2.2.4.jar
cp /home/peterix/minecraft/FTB/libraries/com/mojang/authlib/1.5.16/authlib-1.5.16.jar
cp /home/peterix/minecraft/FTB/libraries/org/apache/logging/log4j/log4j-api/2.0-beta9/log4j-api-2.0-beta9.jar
cp /home/peterix/minecraft/FTB/libraries/org/apache/logging/log4j/log4j-core/2.0-beta9/log4j-core-2.0-beta9.jar
cp /home/peterix/minecraft/FTB/libraries/org/lwjgl/lwjgl/lwjgl/2.9.1/lwjgl-2.9.1.jar
cp /home/peterix/minecraft/FTB/libraries/org/lwjgl/lwjgl/lwjgl_util/2.9.1/lwjgl_util-2.9.1.jar
cp /home/peterix/minecraft/FTB/libraries/tv/twitch/twitch/5.16/twitch-5.16.jar
cp /home/peterix/minecraft/FTB/libraries/net/minecraftforge/forge/1.7.10-10.13.0.1178/forge-1.7.10-10.13.0.1178.jar
cp /home/peterix/minecraft/FTB/libraries/net/minecraft/launchwrapper/1.9/launchwrapper-1.9.jar
cp /home/peterix/minecraft/FTB/libraries/org/ow2/asm/asm-all/4.1/asm-all-4.1.jar
cp /home/peterix/minecraft/FTB/libraries/com/typesafe/akka/akka-actor_2.11/2.3.3/akka-actor_2.11-2.3.3.jar
cp /home/peterix/minecraft/FTB/libraries/com/typesafe/config/1.2.1/config-1.2.1.jar
cp /home/peterix/minecraft/FTB/libraries/org/scala-lang/scala-actors-migration_2.11/1.1.0/scala-actors-migration_2.11-1.1.0.jar
cp /home/peterix/minecraft/FTB/libraries/org/scala-lang/scala-compiler/2.11.1/scala-compiler-2.11.1.jar
cp /home/peterix/minecraft/FTB/libraries/org/scala-lang/plugins/scala-continuations-library_2.11/1.0.2/scala-continuations-library_2.11-1.0.2.jar
cp /home/peterix/minecraft/FTB/libraries/org/scala-lang/plugins/scala-continuations-plugin_2.11.1/1.0.2/scala-continuations-plugin_2.11.1-1.0.2.jar
cp /home/peterix/minecraft/FTB/libraries/org/scala-lang/scala-library/2.11.1/scala-library-2.11.1.jar
cp /home/peterix/minecraft/FTB/libraries/org/scala-lang/scala-parser-combinators_2.11/1.0.1/scala-parser-combinators_2.11-1.0.1.jar
cp /home/peterix/minecraft/FTB/libraries/org/scala-lang/scala-reflect/2.11.1/scala-reflect-2.11.1.jar
cp /home/peterix/minecraft/FTB/libraries/org/scala-lang/scala-swing_2.11/1.0.1/scala-swing_2.11-1.0.1.jar
cp /home/peterix/minecraft/FTB/libraries/org/scala-lang/scala-xml_2.11/1.0.2/scala-xml_2.11-1.0.2.jar
cp /home/peterix/minecraft/FTB/libraries/lzma/lzma/0.0.1/lzma-0.0.1.jar
ext /home/peterix/minecraft/FTB/libraries/org/lwjgl/lwjgl/lwjgl-platform/2.9.1/lwjgl-platform-2.9.1-natives-linux.jar
ext /home/peterix/minecraft/FTB/libraries/net/java/jinput/jinput-platform/2.0.5/jinput-platform-2.0.5-natives-linux.jar
natives /home/peterix/minecraft/FTB/17ForgeTest/natives
cp /home/peterix/minecraft/FTB/versions/1.7.10/1.7.10.jar
launcher onesix
```

Available under the Apache 2.0 license.

## libnbtplusplus
libnbt++ is a free C++ library for Minecraft's file format Named Binary Tag (NBT). It can read and write compressed and uncompressed NBT files and provides a code interface for working with NBT data.

See [github repo](https://github.com/ljfa-ag/libnbtplusplus).

Available either under LGPL version 3 or later.

## pack200
Unpacks pack200 archives (squished, compression-optimized Java jars). This format is only used by Forge to save bandwidth.

A horrible little thing extracted from the depths of the OpenJDK codebase. Please don't look at it, or you will praise Cthulhu for his clean code for the rest of your days.

Available under GPL 2 with classpath exception.

## rainbow
Color functions extracted from [KGuiAddons](https://inqlude.org/libraries/kguiaddons.html). Used for adaptive text coloring.

Available either under LGPL version 2.1 or later.

## xz-embedded
Tiny implementation of LZMA2 de/compression. This format is only used by Forge to save bandwidth.

Public domain.

## LocalPeer
Library for making only one instance of the application run at all times.

BSD licensed, derived from [QtSingleApplication](https://github.com/qtproject/qt-solutions/tree/master/qtsingleapplication).

Changes are made to make the code more generic and useful in less usual conditions.
