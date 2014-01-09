cmake_minimum_required(VERSION 2.8.6)
project(launcher Java)
find_package(Java 1.6 REQUIRED COMPONENTS Development)

include(UseJava)
set(CMAKE_JAVA_JAR_ENTRY_POINT org.multimc.EntryPoint)
set(CMAKE_JAVA_COMPILE_FLAGS -target 1.6 -source 1.6 -Xlint:deprecation -Xlint:unchecked)

set(SRC
	# OSX things
    org/simplericity/macify/eawt/Application.java
    org/simplericity/macify/eawt/ApplicationAdapter.java
    org/simplericity/macify/eawt/ApplicationEvent.java
    org/simplericity/macify/eawt/ApplicationListener.java
    org/simplericity/macify/eawt/DefaultApplication.java

	# legacy applet wrapper thing.
	# The launcher has to be there for silly FML/Forge relauncher.
    net/minecraft/Launcher.java
    org/multimc/legacy/LegacyLauncher.java
    org/multimc/legacy/LegacyFrame.java

	# onesix launcher
    org/multimc/onesix/OneSixLauncher.java

	# generic launcher
	org/multimc/EntryPoint.java
    org/multimc/Launcher.java
    org/multimc/ParseException.java
    org/multimc/Utils.java
    org/multimc/IconLoader.java
)
add_jar(NewLaunch ${SRC})

INSTALL_JAR(NewLaunch "${BINARY_DEST_DIR}/jars")
