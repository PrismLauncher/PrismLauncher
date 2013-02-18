cmake_minimum_required(VERSION 2.8.9)
project(MultiMC)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)


#### Check for machine endianness ####
INCLUDE(TestBigEndian)
TEST_BIG_ENDIAN(BIGENDIAN)
IF(${BIGENDIAN})
     ADD_DEFINITIONS(-DMULTIMC_BIG_ENDIAN)
ENDIF(${BIGENDIAN})

# First, include header overrides
include_directories(hacks)

#### Find the required Qt parts ####
find_package(Qt5Widgets)
find_package(Qt5Network)
#find_package(Qt5Declarative)

include_directories(${Qt5Widgets_INCLUDE_DIRS})

# find ZLIB for quazip
find_package(ZLIB REQUIRED)

# Add quazip
add_subdirectory(quazip)

# Add bspatch
add_subdirectory(patchlib)
include_directories(patchlib)

# add the java launcher
add_subdirectory(launcher)

IF(APPLE)
  # assume clang 4.1.0+, add C++0x/C++11 stuff
  message(STATUS "Using APPLE CMAKE_CXX_FLAGS")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -stdlib=libc++")
ELSEIF(UNIX)
  # assume GCC, add C++0x/C++11 stuff
  message(STATUS "Using UNIX CMAKE_CXX_FLAGS")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
ELSEIF(MINGW)
  message(STATUS "Using MINGW CMAKE_CXX_FLAGS")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x")
ENDIF()

# Set the path where CMake will look for modules.
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")


set(MultiMC_VERSION_MAJOR		5)
set(MultiMC_VERSION_MINOR		0)
set(MultiMC_VERSION_REV			0)

SET(MultiMC_VERSION_BUILD 0 CACHE STRING "Build number.")
message(STATUS "MultiMC build #${MultiMC_VERSION_BUILD}")

IF (DEFINED MultiMC_BUILD_TAG)
	message(STATUS "Build tag: ${MultiMC_BUILD_TAG}")
ELSE ()
	message(STATUS "No build tag specified.")
ENDIF ()

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
	set (MultiMC_ARCH "x64"
		CACHE STRING "Architecture we're building for.")
else()
	set (MultiMC_ARCH "x86"
		CACHE STRING "Architecture we're building for.")
endif()
message (STATUS "Architecture is ${MultiMC_ARCH}")

SET(MultiMC_Extra_Label "")

IF (WIN32)
	SET(MultiMC_JOB_NAME "MultiMC4Windows" CACHE STRING "Jenkins job name.")
ELSEIF(UNIX AND APPLE)
	SET(MultiMC_JOB_NAME "MultiMC4OSX" CACHE STRING "Jenkins job name.")
	# This is here because the scheme doesn't exactly apply to every kind of build...
	SET(MultiMC_Extra_Label ",label=osx")
ELSE()
	SET(MultiMC_JOB_NAME "MultiMC4Linux" CACHE STRING "Jenkins job name.")
ENDIF()

SET(MultiMC_JOB_URL "http://ci.forkk.net/job/${MultiMC_JOB_NAME}/arch=${MultiMC_ARCH}${MultiMC_Extra_Label}/"
    CACHE STRING "URL of the jenkins job to pull updates from.")
message(STATUS "Job URL: ${MultiMC_JOB_URL}")

configure_file("${PROJECT_SOURCE_DIR}/config.h.in"
			   "${PROJECT_BINARY_DIR}/config.h")


SET(MULTIMC_SOURCES
main.cpp

data/appsettings.cpp
data/inifile.cpp
#data/instancemodel.cpp
data/version.cpp
data/userinfo.cpp
data/loginresponse.cpp

data/inst/instance.cpp
data/inst/instancelist.cpp

data/version/instversion.cpp
data/version/instversionlist.cpp

gui/mainwindow.cpp
gui/modeditwindow.cpp
gui/settingsdialog.cpp
gui/newinstancedialog.cpp
gui/logindialog.cpp
gui/taskdialog.cpp

util/pathutils.cpp
util/osutils.cpp

java/javautils.cpp
java/annotations.cpp

tasks/task.cpp
tasks/logintask.cpp
)

SET(MULTIMC_HEADERS
gui/mainwindow.h
gui/modeditwindow.h
gui/settingsdialog.h
gui/newinstancedialog.h
gui/logindialog.h
gui/taskdialog.h

data/appsettings.h
data/inifile.h
data/instancemodel.h
data/version.h
data/userinfo.h
data/loginresponse.h
data/siglist.h
data/siglist_impl.h

data/inst/instance.h
data/inst/instancelist.h

data/version/instversion.h
data/version/instversionlist.h

util/apputils.h
util/pathutils.h
util/osutils.h

multimc_pragma.h

java/annotations.h
java/classfile.h
java/constants.h
java/endian.h
java/errors.h
java/javautils.h
java/membuffer.h

tasks/task.h
tasks/logintask.h
)

SET(MULTIMC5_UIS
gui/mainwindow.ui
gui/modeditwindow.ui
gui/settingsdialog.ui
gui/newinstancedialog.ui
gui/logindialog.ui
gui/taskdialog.ui
)

################################ Install ################################

################ ICNS File ################

IF(APPLE)
	SET(MACOSX_BUNDLE_ICON_FILE MultiMC.icns) 
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/MultiMC.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
	SET(MULTIMC_SOURCES ${MULTIMC_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/MultiMC.icns)
ENDIF(APPLE)


################ Build ################

IF (WIN32)
    SET(MultiMC_LINK_ADDITIONAL_LIBS ${MultiMC_LINK_ADDITIONAL_LIBS}
    	Qt5::WinMain
    )
ENDIF (WIN32)

SET_SOURCE_FILES_PROPERTIES(resources/MultiMCLauncher.jar GENERATED)

QT5_WRAP_UI(MULTIMC_UI ${MULTIMC5_UIS})
QT5_ADD_RESOURCES(MULTIMC_QRC multimc.qrc)

add_executable(MultiMC MACOSX_BUNDLE WIN32 ${MULTIMC_SOURCES} ${MULTIMC_HEADERS} ${MULTIMC_UI} ${MULTIMC_QRC})
qt5_use_modules(MultiMC Widgets Network)
target_link_libraries(MultiMC quazip patchlib ${MultiMC_LINK_ADDITIONAL_LIBS})
add_dependencies(MultiMC MultiMCLauncher)


################ Dirs ################

SET(PLUGIN_DEST_DIR bin)
SET(QTCONF_DEST_DIR bin)
SET(APPS "\${CMAKE_INSTALL_PREFIX}/bin/MultiMC")

IF(WIN32)
	#SET(PLUGIN_DEST_DIR .)
	#SET(QTCONF_DEST_DIR .)
	SET(APPS "\${CMAKE_INSTALL_PREFIX}/bin/MultiMC.exe")
ENDIF()
IF(APPLE)
	SET(PLUGIN_DEST_DIR MultiMC.app/Contents/MacOS)
	SET(QTCONF_DEST_DIR MultiMC.app/Contents/Resources)
	SET(APPS "\${CMAKE_INSTALL_PREFIX}/MultiMC.app")
ENDIF()

SET(QT_PLUGINS_DIR ${Qt5_DIR}/plugins)
SET(QT_LIBRARY_DIRS ${Qt5_DIR}/lib)


################ OS X Bundle Info ################

IF(APPLE)
	SET(MACOSX_BUNDLE_BUNDLE_NAME "MultiMC")
	SET(MACOSX_BUNDLE_INFO_STRING "MultiMC Minecraft launcher and management utility.")
	SET(MACOSX_BUNDLE_BUNDLE_VERSION "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}.${MultiMC_VERSION_REV}.${MultiMC_VERSION_BUILD}")
	#SET(MACOSX_BUNDLE_GUI_IDENTIFIER "")
	SET(MACOSX_BUNDLE_ICON_FILE MultiMC.icns)
ENDIF(APPLE)

################ Install ################

# Executable
IF(WIN32)
INSTALL(TARGETS MultiMC 
	BUNDLE DESTINATION . COMPONENT Runtime
	RUNTIME DESTINATION bin COMPONENT Runtime
)
ENDIF()
IF(UNIX)
IF(APPLE)
INSTALL(TARGETS MultiMC 
	BUNDLE DESTINATION . COMPONENT Runtime
	RUNTIME DESTINATION MultiMC.app/Contents/MacOS COMPONENT Runtime
)
ELSE()
INSTALL(TARGETS MultiMC 
	BUNDLE DESTINATION . COMPONENT Runtime
	RUNTIME DESTINATION bin COMPONENT Runtime
)
ENDIF()
ENDIF()

# Plugins

# Image formats
INSTALL(DIRECTORY "${QT_PLUGINS_DIR}/imageformats" DESTINATION ${PLUGIN_DEST_DIR} COMPONENT Runtime)

# Platform plugins
INSTALL(DIRECTORY "${QT_PLUGINS_DIR}/platforms" DESTINATION ${PLUGIN_DEST_DIR} COMPONENT Runtime)

# qtconf
INSTALL(CODE "
	FILE(WRITE \"\${CMAKE_INSTALL_PREFIX}/${QTCONF_DEST_DIR}/qt.conf\" \"\")
	" COMPONENT Runtime)


# Dirs to look for dependencies.
SET(DIRS ${QT_LIBRARY_DIRS})

INSTALL(CODE "
	file(GLOB_RECURSE QTPLUGINS
		\"\${CMAKE_INSTALL_PREFIX}/${PLUGIN_DEST_DIR}/plugins/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
	include(BundleUtilities)
	fixup_bundle(\"${APPS}\" \"\${QTPLUGINS}\" \"${DIRS}\")
	" COMPONENT Runtime)


#GET_TARGET_PROPERTY(BINARY_LOCATION MultiMC LOCATION)
#CONFIGURE_FILE(
#   "${CMAKE_CURRENT_SOURCE_DIR}/dependencies.cmake.in"
#   "${CMAKE_CURRENT_BINARY_DIR}/dependencies.cmake"
#   @ONLY
#   )
#INSTALL(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/dependencies.cmake")


################ Package ################

# Package with CPack
IF(UNIX)
    if(APPLE)
        SET(CPACK_GENERATOR "ZIP")
    else()
        SET(CPACK_GENERATOR "TGZ")
    endif()
ELSEIF(WIN32)
    SET(CPACK_GENERATOR "ZIP")
ENDIF()
SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)

SET(CPACK_PACKAGE_NAME "MultiMC 5")
SET(CPACK_PACKAGE_VENDOR "")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MultiMC - Minecraft launcher and management tool.")
SET(CPACK_PACKAGE_VERSION "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}.${MultiMC_VERSION_REV}.${MultiMC_VERSION_BUILD}")
SET(CPACK_PACKAGE_VERSION_MAJOR ${MultiMC_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${MultiMC_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${MultiMC_VERSION_REV})

IF(CPACK_GENERATOR STREQUAL "NSIS")
SET(CPACK_PACKAGE_FILE_NAME "Setup-MultiMC")
ELSE()
SET(CPACK_PACKAGE_FILE_NAME "MultiMC")
ENDIF()

IF(WIN32)
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "MultiMC 5")
ENDIF()

INCLUDE(CPack)
