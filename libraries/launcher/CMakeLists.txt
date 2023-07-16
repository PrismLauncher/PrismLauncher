cmake_minimum_required(VERSION 3.9.4)
project(launcher Java)
find_package(Java 1.7 REQUIRED COMPONENTS Development)

include(UseJava)
set(CMAKE_JAVA_JAR_ENTRY_POINT org.prismlauncher.EntryPoint)
set(CMAKE_JAVA_COMPILE_FLAGS -target 7 -source 7)

set(SRC
    org/prismlauncher/EntryPoint.java
    org/prismlauncher/launcher/Launcher.java
    org/prismlauncher/launcher/impl/AbstractLauncher.java
    org/prismlauncher/launcher/impl/StandardLauncher.java
    org/prismlauncher/launcher/impl/legacy/LegacyLauncher.java
    org/prismlauncher/launcher/impl/legacy/LegacyFrame.java
    org/prismlauncher/exception/ParameterNotFoundException.java
    org/prismlauncher/exception/ParseException.java
    org/prismlauncher/utils/Parameters.java
    org/prismlauncher/utils/ReflectionUtils.java
    org/prismlauncher/utils/logging/Level.java
    org/prismlauncher/utils/logging/Log.java
    net/minecraft/Launcher.java
)
add_jar(NewLaunch ${SRC})
install_jar(NewLaunch "${JARS_DEST_DIR}")
