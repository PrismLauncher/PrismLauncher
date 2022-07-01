cmake_minimum_required(VERSION 3.9.4)
project(launcher Java)
find_package(Java 1.7 REQUIRED COMPONENTS Development)

include(UseJava)
set(CMAKE_JAVA_JAR_ENTRY_POINT org.polymc.EntryPoint)
set(CMAKE_JAVA_COMPILE_FLAGS -target 7 -source 7 -Xlint:deprecation -Xlint:unchecked)

set(SRC
    org/polymc/EntryPoint.java
    org/polymc/Launcher.java
    org/polymc/LauncherFactory.java
    org/polymc/impl/OneSixLauncher.java
    org/polymc/applet/LegacyFrame.java
    org/polymc/exception/ParameterNotFoundException.java
    org/polymc/exception/ParseException.java
    org/polymc/utils/Parameters.java
    org/polymc/utils/Utils.java
    net/minecraft/Launcher.java
)
add_jar(NewLaunch ${SRC})
install_jar(NewLaunch "${JARS_DEST_DIR}")
