cmake_minimum_required(VERSION 3.9.4)
project(launcher Java)
find_package(Java 1.7 REQUIRED COMPONENTS Development)

include(UseJava)
set(CMAKE_JAVA_JAR_ENTRY_POINT org.prismlauncher.EntryPoint)
set(CMAKE_JAVA_COMPILE_FLAGS -target 7 -source 7 -Xlint:deprecation -Xlint:unchecked)

set(SRC
    org/prismlauncher/EntryPoint.java
    org/prismlauncher/Launcher.java
    org/prismlauncher/LauncherFactory.java
    org/prismlauncher/impl/OneSixLauncher.java
    org/prismlauncher/applet/LegacyFrame.java
    org/prismlauncher/exception/ParameterNotFoundException.java
    org/prismlauncher/exception/ParseException.java
    org/prismlauncher/utils/Parameters.java
    org/prismlauncher/utils/Utils.java
    net/minecraft/Launcher.java
    org/prismlauncher/log4jinjector/Log4JInjector20.java
)
add_jar(
        NewLaunch ${SRC}
        INCLUDE_JARS ${CMAKE_CURRENT_SOURCE_DIR}/compilelibs/log4j-api-2.0-beta9-fixed.jar
        ${CMAKE_CURRENT_SOURCE_DIR}/compilelibs/log4j-core-2.0-beta9-fixed.jar
)
install_jar(NewLaunch "${JARS_DEST_DIR}")
