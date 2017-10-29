cmake_minimum_required(VERSION 3.1)
project(launcher Java)
find_package(Java 1.6 REQUIRED COMPONENTS Development)

include(UseJava)
set(CMAKE_JAVA_JAR_ENTRY_POINT org.multimc.EntryPoint)
set(CMAKE_JAVA_COMPILE_FLAGS -target 1.6 -source 1.6 -Xlint:deprecation -Xlint:unchecked)

set(SRC
	# legacy applet wrapper thing.
	# The launcher has to be there for silly FML/Forge relauncher.
	net/minecraft/Launcher.java
	org/multimc/LegacyFrame.java

	# generic launcher
	org/multimc/EntryPoint.java
	org/multimc/Launcher.java
	org/multimc/LaunchParams.java
	org/multimc/ParseException.java
	org/multimc/Utils.java

	org/multimc/onesix/OneSixLauncher.java
)
add_jar(NewLaunch ${SRC})

