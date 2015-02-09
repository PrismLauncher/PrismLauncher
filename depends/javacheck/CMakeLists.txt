cmake_minimum_required(VERSION 2.8.6)
project(launcher Java)
find_package(Java 1.6 REQUIRED COMPONENTS Development)

include(UseJava)
set(CMAKE_JAVA_JAR_ENTRY_POINT JavaCheck)
set(CMAKE_JAVA_COMPILE_FLAGS -target 1.6 -source 1.6 -Xlint:deprecation -Xlint:unchecked)

set(SRC
	JavaCheck.java
)

add_jar(JavaCheck ${SRC})
