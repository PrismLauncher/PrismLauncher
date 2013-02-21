cmake_minimum_required(VERSION 2.8.9)
project(MultiMC)

######## Set CMake options ########
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

# Output all executables and shared libs in the main build folder, not in subfolders.
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})


################################ INCLUDE LIBRARIES ################################

# First, include header overrides
include_directories(hacks)


######## 3rd Party Libs ########

# Find the required Qt parts
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)

include_directories(${Qt5Widgets_INCLUDE_DIRS})

# Find ZLIB for quazip
find_package(ZLIB REQUIRED)


######## Included Libs ########

# Add quazip
add_subdirectory(quazip)

# Add bspatch
add_subdirectory(patchlib)
include_directories(patchlib)

# Add the java launcher
add_subdirectory(launcher)


######## MultiMC Libs ########

# Add the util library.
add_subdirectory(libutil)
include_directories(${LIBMMCUTIL_INCLUDE_DIR})

# Add the settings library.
add_subdirectory(libsettings)
include_directories(${LIBMMCSETTINGS_INCLUDE_DIR})

# Add the instance library.
add_subdirectory(libinstance)
include_directories(${LIBMMCINST_INCLUDE_DIR})

# Add the stdinstance plugin.
add_subdirectory(plugins/stdinstance)



################################ SET UP BUILD OPTIONS ################################

######## Check endianness ########
INCLUDE(TestBigEndian)
TEST_BIG_ENDIAN(BIGENDIAN)
IF(${BIGENDIAN})
     ADD_DEFINITIONS(-DMULTIMC_BIG_ENDIAN)
ENDIF(${BIGENDIAN})


######## Set module path ########
SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")


######## Set compiler flags ########
IF(APPLE)
	# assume clang 4.1.0+, add C++0x/C++11 stuff
	message(STATUS "Using APPLE CMAKE_CXX_FLAGS")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -stdlib=libc++")
ELSEIF(UNIX)
	# assume GCC, add C++0x/C++11 stuff
	MESSAGE(STATUS "Using UNIX CMAKE_CXX_FLAGS")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
ELSEIF(MINGW)
	MESSAGE(STATUS "Using MINGW CMAKE_CXX_FLAGS")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x")
ENDIF()


######## Set version numbers ########
SET(MultiMC_VERSION_MAJOR		5)
SET(MultiMC_VERSION_MINOR		0)
SET(MultiMC_VERSION_REV			0)

# Jenkins build number.
SET(MultiMC_VERSION_BUILD 0 CACHE STRING "Build number.")
MESSAGE(STATUS "MultiMC build #${MultiMC_VERSION_BUILD}")


######## Set Jenkins info ########
# Jenkins build tag
IF(DEFINED MultiMC_BUILD_TAG)
	MESSAGE(STATUS "Build tag: ${MultiMC_BUILD_TAG}")
ELSE()
	MESSAGE(STATUS "No build tag specified.")
ENDIF()

# Architecture detection
IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
	SET(MultiMC_ARCH "x64" CACHE STRING "Architecture we're building for.")
ELSE()
	SET(MultiMC_ARCH "x86" CACHE STRING "Architecture we're building for.")
ENDIF()
MESSAGE(STATUS "Architecture is ${MultiMC_ARCH}")

# Jenkins job name
IF(WIN32)
	SET(MultiMC_JOB_NAME "MultiMC5Windows" CACHE STRING "Jenkins job name.")
ELSEIF(UNIX AND APPLE)
	SET(MultiMC_JOB_NAME "MultiMC5OSX" CACHE STRING "Jenkins job name.")
ELSE()
	SET(MultiMC_JOB_NAME "MultiMC5Linux" CACHE STRING "Jenkins job name.")
ENDIF()

# Jenkins URL
SET(MultiMC_JOB_URL "http://ci.forkk.net/job/${MultiMC_JOB_NAME}/arch=${MultiMC_ARCH}${MultiMC_Extra_Label}/"
    CACHE STRING "URL of the jenkins job to pull updates from.")
MESSAGE(STATUS "Job URL: ${MultiMC_JOB_URL}")

######## Configure header ########
configure_file("${PROJECT_SOURCE_DIR}/config.h.in"
			   "${PROJECT_BINARY_DIR}/config.h")


################################ FILES ################################

######## Headers ########
SET(MULTIMC_HEADERS
gui/mainwindow.h
gui/modeditwindow.h
gui/settingsdialog.h
gui/newinstancedialog.h
gui/logindialog.h
gui/taskdialog.h

data/version.h
data/userinfo.h
data/loginresponse.h

data/plugin/pluginmanager.h

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


######## Sources ########
SET(MULTIMC_SOURCES
main.cpp

data/version.cpp
data/userinfo.cpp
data/loginresponse.cpp

data/plugin/pluginmanager.cpp

gui/mainwindow.cpp
gui/modeditwindow.cpp
gui/settingsdialog.cpp
gui/newinstancedialog.cpp
gui/logindialog.cpp
gui/taskdialog.cpp

java/javautils.cpp
java/annotations.cpp

tasks/task.cpp
tasks/logintask.cpp
)


######## UIs ########
SET(MULTIMC_UIS
gui/mainwindow.ui
gui/modeditwindow.ui
gui/settingsdialog.ui
gui/newinstancedialog.ui
gui/logindialog.ui
gui/taskdialog.ui
)


######## Windows resource files ########
IF(WIN32)
SET(MULTIMC_RCS multimc.rc)
ENDIF()


################################ COMPILE ################################

# ICNS file for OS X
IF(APPLE)
	SET(MACOSX_BUNDLE_ICON_FILE MultiMC.icns) 
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/MultiMC.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
	SET(MULTIMC_SOURCES ${MULTIMC_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/MultiMC.icns)
ENDIF(APPLE)

# Link additional libraries
IF(WIN32)
    SET(MultiMC_LINK_ADDITIONAL_LIBS ${MultiMC_LINK_ADDITIONAL_LIBS}
    	Qt5::WinMain # Link WinMain
    )
ENDIF(WIN32)

# Tell CMake that MultiMCLauncher.jar is generated.
SET_SOURCE_FILES_PROPERTIES(resources/MultiMCLauncher.jar GENERATED)

# Qt 5 stuff
QT5_WRAP_UI(MULTIMC_UI ${MULTIMC_UIS})
QT5_ADD_RESOURCES(MULTIMC_QRC multimc.qrc)

# Add executable
ADD_EXECUTABLE(MultiMC MACOSX_BUNDLE WIN32
	${MULTIMC_SOURCES} ${MULTIMC_HEADERS} ${MULTIMC_UI} ${MULTIMC_QRC} ${MULTIMC_RCS})

# Link
QT5_USE_MODULES(MultiMC Widgets Network)
TARGET_LINK_LIBRARIES(MultiMC quazip patchlib 
libmmcutil libmmcsettings libmmcinst
${MultiMC_LINK_ADDITIONAL_LIBS})
ADD_DEPENDENCIES(MultiMC MultiMCLauncher libmmcutil libmmcsettings libmmcinst)


################################ INSTALLATION AND PACKAGING ################################

######## Plugin and library folders ########

SET(PLUGIN_DEST_DIR bin)
SET(QTCONF_DEST_DIR bin)
SET(APPS "\${CMAKE_INSTALL_PREFIX}/bin/MultiMC")

IF(WIN32)
	SET(PLUGIN_DEST_DIR .)
	SET(QTCONF_DEST_DIR .)
	SET(APPS "\${CMAKE_INSTALL_PREFIX}/MultiMC.exe")
ENDIF()

IF(APPLE)
	SET(PLUGIN_DEST_DIR MultiMC.app/Contents/MacOS)
	SET(QTCONF_DEST_DIR MultiMC.app/Contents/Resources)
	SET(APPS "\${CMAKE_INSTALL_PREFIX}/MultiMC.app")
ENDIF()

SET(QT_PLUGINS_DIR ${Qt5_DIR}/plugins)
SET(QT_LIBRARY_DIRS ${Qt5_DIR}/lib)


######## OS X Bundle Info ########

IF(APPLE)
	SET(MACOSX_BUNDLE_BUNDLE_NAME "MultiMC")
	SET(MACOSX_BUNDLE_INFO_STRING "MultiMC Minecraft launcher and management utility.")
	SET(MACOSX_BUNDLE_BUNDLE_VERSION 
		"${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}.${MultiMC_VERSION_REV}.${MultiMC_VERSION_BUILD}")
	#SET(MACOSX_BUNDLE_GUI_IDENTIFIER "")
	SET(MACOSX_BUNDLE_ICON_FILE MultiMC.icns)
ENDIF(APPLE)

######## Install ########

#### Executable ####
IF(WIN32)
INSTALL(TARGETS MultiMC 
	BUNDLE DESTINATION . COMPONENT Runtime
	RUNTIME DESTINATION . COMPONENT Runtime
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


#### Plugins ####

# Image formats
INSTALL(DIRECTORY "${QT_PLUGINS_DIR}/imageformats" DESTINATION ${PLUGIN_DEST_DIR} COMPONENT Runtime)

# Platform plugins
INSTALL(DIRECTORY "${QT_PLUGINS_DIR}/platforms" DESTINATION ${PLUGIN_DEST_DIR} COMPONENT Runtime)

# qtconf
INSTALL(CODE "
	FILE(WRITE \"\${CMAKE_INSTALL_PREFIX}/${QTCONF_DEST_DIR}/qt.conf\" \"\")
	" COMPONENT Runtime)


# Dirs to look for dependencies.
SET(DIRS "${QT_LIBRARY_DIRS}
${CMAKE_BINARY_DIR}/libutil
${CMAKE_BINARY_DIR}/libsettings
${CMAKE_BINARY_DIR}/libinstance")
message(STATUS "${DIRS}")

INSTALL(CODE "
	file(GLOB_RECURSE QTPLUGINS
		\"\${CMAKE_INSTALL_PREFIX}/${PLUGIN_DEST_DIR}/plugins/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
	include(BundleUtilities)
	fixup_bundle(\"${APPS}\" \"\${QTPLUGINS}\" \"${DIRS}\")
	" COMPONENT Runtime)


######## Package ########

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
