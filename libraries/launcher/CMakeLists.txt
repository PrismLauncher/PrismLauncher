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
    org/prismlauncher/exception/ParameterNotFoundException.java
    org/prismlauncher/exception/ParseException.java
    org/prismlauncher/utils/Parameters.java
    org/prismlauncher/utils/ReflectionUtils.java
    org/prismlauncher/utils/logging/Level.java
    org/prismlauncher/utils/logging/Log.java
    org/prismlauncher/legacy/LegacyProxy.java
)

set(LEGACY_SRC
    legacy/org/prismlauncher/legacy/LegacyFrame.java
    legacy/org/prismlauncher/legacy/LegacyLauncher.java
    legacy/org/prismlauncher/legacy/fix/online/Handler.java
    legacy/org/prismlauncher/legacy/fix/online/OnlineFixes.java
    legacy/org/prismlauncher/legacy/fix/online/OnlineModeFix.java
    legacy/org/prismlauncher/legacy/fix/online/SkinFix.java
    legacy/org/prismlauncher/legacy/utils/Base64.java
    legacy/org/prismlauncher/legacy/utils/api/MojangApi.java
    legacy/org/prismlauncher/legacy/utils/api/Texture.java
    legacy/org/prismlauncher/legacy/utils/json/JsonParseException.java
    legacy/org/prismlauncher/legacy/utils/json/JsonParser.java
    legacy/org/prismlauncher/legacy/utils/url/ByteArrayUrlConnection.java
    legacy/org/prismlauncher/legacy/utils/url/UrlUtils.java
    legacy/net/minecraft/Launcher.java
    legacy/org/prismlauncher/legacy/LegacyProxy.java
)

add_jar(NewLaunch ${SRC})
add_jar(NewLaunchLegacy ${LEGACY_SRC} INCLUDE_JARS NewLaunch)
install_jar(NewLaunch "${JARS_DEST_DIR}")
install_jar(NewLaunchLegacy "${JARS_DEST_DIR}")
