cmake_minimum_required(VERSION 3.1)
project(launcher Java)
find_package(Java 1.6 REQUIRED COMPONENTS Development)

include(UseJava)
set(CMAKE_JAVA_JAR_ENTRY_POINT org.multimc.EntryPoint)
set(CMAKE_JAVA_COMPILE_FLAGS -target 1.6 -source 1.6 -Xlint:deprecation -Xlint:unchecked)

set(SRC
	org/multimc/EntryPoint.java
	org/multimc/Launcher.java
	org/multimc/LegacyFrame.java
	org/multimc/NotFoundException.java
	org/multimc/ParamBucket.java
	org/multimc/ParseException.java
	org/multimc/Utils.java
	org/multimc/onesix/OneSixLauncher.java
	net/minecraft/Launcher.java
)
add_jar(NewLaunch ${SRC})
install_jar(NewLaunch "${JARS_DEST_DIR}")
