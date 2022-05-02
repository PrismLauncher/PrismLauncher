cmake_minimum_required(VERSION 3.9.4)
project(launcher Java)
find_package(Java 1.7 REQUIRED COMPONENTS Development)

include(UseJava)
set(CMAKE_JAVA_JAR_ENTRY_POINT org.multimc.EntryPoint)
set(CMAKE_JAVA_COMPILE_FLAGS -target 8 -source 8 -Xlint:deprecation -Xlint:unchecked)

set(SRC
    org/multimc/EntryPoint.java
    org/multimc/Launcher.java
    org/multimc/LauncherFactory.java
    org/multimc/impl/OneSixLauncher.java
    org/multimc/applet/LegacyFrame.java
    org/multimc/exception/ParameterNotFoundException.java
    org/multimc/exception/ParseException.java
    org/multimc/utils/ParamBucket.java
    org/multimc/utils/Utils.java
    net/minecraft/Launcher.java
)
add_jar(NewLaunch ${SRC})
install_jar(NewLaunch "${JARS_DEST_DIR}")
