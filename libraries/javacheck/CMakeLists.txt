cmake_minimum_required(VERSION 3.9.4)
project(launcher Java)
find_package(Java 1.8 REQUIRED COMPONENTS Development)

include(UseJava)
set(CMAKE_JAVA_JAR_ENTRY_POINT JavaCheck)
set(CMAKE_JAVA_COMPILE_FLAGS -target 8 -source 8 -Xlint:deprecation -Xlint:unchecked)

set(SRC
    JavaCheck.java
)

add_jar(JavaCheck ${SRC})
install_jar(JavaCheck "${JARS_DEST_DIR}")
