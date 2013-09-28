cmake_minimum_required(VERSION 2.8.9)

# In Qt 5.1+ we have our own main() function, don't autolink to qtmain on Windows
cmake_policy(SET CMP0020 OLD)

project(MultiMC)

######## Set CMake options ########
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(FILES_TO_TRANSLATE )

# Output all executables and shared libs in the main build folder, not in subfolders.
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

IF(UNIX)
	SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
ENDIF()

######## Set compiler flags ########
IF(APPLE)
	message(STATUS "Using APPLE CMAKE_CXX_FLAGS")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
ELSEIF(UNIX)
	# assume GCC, add C++0x/C++11 stuff
	MESSAGE(STATUS "Using UNIX CMAKE_CXX_FLAGS")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
ELSEIF(MINGW)
	MESSAGE(STATUS "Using MINGW CMAKE_CXX_FLAGS")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x")
ENDIF()

################################ INCLUDE LIBRARIES ################################

######## 3rd Party Libs ########

# Find the required Qt parts
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5LinguistTools REQUIRED)

include_directories(${Qt5Widgets_INCLUDE_DIRS})

######## Included Libs ########

# Add quazip
add_subdirectory(depends/quazip)
include_directories(depends/quazip)

# Add lzma
#add_subdirectory(depends/lzma)
#include_directories(depends/lzma/include)

# Add the java launcher
add_subdirectory(depends/launcher)

######## MultiMC Libs ########

# Add the util library.
add_subdirectory(depends/util)
include_directories(${LIBUTIL_INCLUDE_DIR})

# Add the settings library.
add_subdirectory(depends/settings)
include_directories(${LIBSETTINGS_INCLUDE_DIR})

# Add the group view library.
add_subdirectory(depends/groupview)
include_directories(${LIBGROUPVIEW_INCLUDE_DIR})

#pack 200
#add_subdirectory(depends/pack200)

################################ SET UP BUILD OPTIONS ################################

######## Check endianness ########
INCLUDE(TestBigEndian)
TEST_BIG_ENDIAN(BIGENDIAN)
IF(${BIGENDIAN})
	ADD_DEFINITIONS(-DMULTIMC_BIG_ENDIAN)
ENDIF(${BIGENDIAN})


######## Set module path ########
SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")


######## Set version numbers ########
SET(MultiMC_VERSION_MAJOR		5)
SET(MultiMC_VERSION_MINOR		0)
SET(MultiMC_VERSION_REV			0)

# Jenkins build number
SET(MultiMC_VERSION_BUILD 0 CACHE STRING "Build number.")
MESSAGE(STATUS "MultiMC build #${MultiMC_VERSION_BUILD}")

# Check the current Git commit
execute_process(COMMAND git rev-parse HEAD
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	RESULT_VARIABLE GIT_COMMIT_CHECK_RESULTVAR
	OUTPUT_VARIABLE GIT_COMMIT_CHECK_OUTVAR
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

# If Git executed successfully
IF(GIT_COMMIT_CHECK_RESULTVAR EQUAL 0)
	SET(MultiMC_GIT_COMMIT "${GIT_COMMIT_CHECK_OUTVAR}")
	MESSAGE(STATUS "Git commit: ${MultiMC_GIT_COMMIT}")
ELSE()
	SET(MultiMC_GIT_COMMIT "Unknown")
	MESSAGE(STATUS "Failed to check Git commit. ${GIT_COMMIT_CHECK_RESULTVAR}")
ENDIF()


######## Set Jenkins info ########
# Jenkins build tag
IF(DEFINED MultiMC_BUILD_TAG)
	MESSAGE(STATUS "Build tag: ${MultiMC_BUILD_TAG}")
ELSE()
	MESSAGE(STATUS "No build tag specified.")
	SET(MultiMC_BUILD_TAG custom)
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
				"${PROJECT_BINARY_DIR}/include/config.h")


######## Other Stuff ########

ADD_DEFINITIONS(-DQUAZIP_STATIC)
ADD_DEFINITIONS(-DLIBSETTINGS_STATIC)
ADD_DEFINITIONS(-DLIBUTIL_STATIC)
ADD_DEFINITIONS(-DLIBGROUPVIEW_STATIC)

################################ FILES ################################

######## Sources and headers ########
SET(MULTIMC_SOURCES
# Application base
MultiMC.h
MultiMC.cpp
MultiMCVersion.h

# GUI
gui/mainwindow.h
gui/mainwindow.cpp
gui/settingsdialog.h
gui/settingsdialog.cpp
gui/newinstancedialog.h
gui/newinstancedialog.cpp
gui/logindialog.h
gui/logindialog.cpp
gui/ProgressDialog.h
gui/ProgressDialog.cpp
gui/aboutdialog.h
gui/aboutdialog.cpp
gui/consolewindow.h
gui/consolewindow.cpp
gui/instancedelegate.h
gui/instancedelegate.cpp
gui/versionselectdialog.h
gui/versionselectdialog.cpp
gui/lwjglselectdialog.h
gui/lwjglselectdialog.cpp
gui/instancesettings.h
gui/instancesettings.cpp
gui/IconPickerDialog.h
gui/IconPickerDialog.cpp
gui/LegacyModEditDialog.h
gui/LegacyModEditDialog.cpp
gui/OneSixModEditDialog.h
gui/OneSixModEditDialog.cpp
gui/ModEditDialogCommon.h
gui/ModEditDialogCommon.cpp
gui/ModListView.h
gui/ModListView.cpp
gui/LabeledToolButton.h
gui/LabeledToolButton.cpp
gui/EditNotesDialog.h
gui/EditNotesDialog.cpp

# Base classes and infrastructure
logic/BaseVersion.h
logic/MinecraftVersion.h
logic/InstanceFactory.h
logic/InstanceFactory.cpp
logic/BaseUpdate.h
logic/BaseUpdate.cpp
logic/BaseInstance.h
logic/BaseInstance.cpp
logic/BaseInstance_p.h

logic/MinecraftProcess.h
logic/MinecraftProcess.cpp
logic/Mod.h
logic/Mod.cpp
logic/ModList.h
logic/ModList.cpp

# Basic instance launcher for starting from terminal
logic/InstanceLauncher.h
logic/InstanceLauncher.cpp

# network stuffs
logic/net/Download.h
logic/net/FileDownload.h
logic/net/FileDownload.cpp
logic/net/ByteArrayDownload.h
logic/net/ByteArrayDownload.cpp
logic/net/CacheDownload.h
logic/net/CacheDownload.cpp
logic/net/DownloadJob.h
logic/net/DownloadJob.cpp
logic/net/HttpMetaCache.h
logic/net/HttpMetaCache.cpp
logic/net/LoginTask.h
logic/net/LoginTask.cpp

# legacy instances
logic/LegacyInstance.h
logic/LegacyInstance.cpp
logic/LegacyInstance_p.h
logic/LegacyUpdate.h
logic/LegacyUpdate.cpp
logic/LegacyForge.h
logic/LegacyForge.cpp

# 1.6 instances
logic/OneSixAssets.h
logic/OneSixAssets.cpp
logic/OneSixInstance.h
logic/OneSixInstance.cpp
logic/OneSixInstance_p.h
logic/OneSixUpdate.h
logic/OneSixUpdate.cpp
logic/OneSixVersion.h
logic/OneSixVersion.cpp
logic/OneSixLibrary.h
logic/OneSixLibrary.cpp
logic/OneSixRule.h
logic/OneSixRule.cpp
logic/OpSys.h
logic/OpSys.cpp
logic/ForgeInstaller.h
logic/ForgeInstaller.cpp

# Nostalgia
logic/NostalgiaInstance.h
logic/NostalgiaInstance.cpp

# Lists
logic/lists/InstanceList.h
logic/lists/InstanceList.cpp
logic/lists/IconList.h
logic/lists/IconList.cpp
logic/lists/BaseVersionList.h
logic/lists/BaseVersionList.cpp
logic/lists/MinecraftVersionList.h
logic/lists/MinecraftVersionList.cpp
logic/lists/LwjglVersionList.h
logic/lists/LwjglVersionList.cpp
logic/lists/ForgeVersionList.h
logic/lists/ForgeVersionList.cpp

# misc model/view
logic/EnabledItemFilter.h
logic/EnabledItemFilter.cpp

# Tasks
logic/tasks/ProgressProvider.h
logic/tasks/Task.h
logic/tasks/Task.cpp


)


######## UIs ########
SET(MULTIMC_UIS
gui/mainwindow.ui
gui/settingsdialog.ui
gui/newinstancedialog.ui
gui/logindialog.ui
gui/aboutdialog.ui
gui/consolewindow.ui
gui/versionselectdialog.ui
gui/lwjglselectdialog.ui
gui/instancesettings.ui

gui/ProgressDialog.ui
gui/IconPickerDialog.ui
gui/LegacyModEditDialog.ui
gui/OneSixModEditDialog.ui
gui/EditNotesDialog.ui
)

set (FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${MULTIMC_SOURCES} ${MULTIMC_UIS})


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
	${MULTIMC_SOURCES} ${MULTIMC_UI} ${MULTIMC_QRC} ${MULTIMC_RCS})

# Link
QT5_USE_MODULES(MultiMC Widgets Network Xml)
TARGET_LINK_LIBRARIES(MultiMC quazip lzma libUtil libSettings libGroupView 
${MultiMC_LINK_ADDITIONAL_LIBS})
#ADD_DEPENDENCIES(MultiMC MultiMCLauncher libUtil libSettings libGroupView)
ADD_DEPENDENCIES(MultiMC MultiMCLauncher)

option(BUILD_KEYRING_TEST "Build the simple keyring test binary" OFF)
IF(BUILD_KEYRING_TEST)
	# test.cpp
	ADD_EXECUTABLE(Test test.cpp)
	QT5_USE_MODULES(Test Core)
	TARGET_LINK_LIBRARIES(Test libUtil libSettings)
ENDIF()

################################ INSTALLATION AND PACKAGING ################################
# use QtCreator's QTDIR var
IF(DEFINED ENV{QTDIR})
	SET(Qt5_DIR $ENV{QTDIR})
ENDIF()

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
	RUNTIME DESTINATION . COMPONENT Runtime
)
ENDIF()
ENDIF()


#### Plugins ####

# # Image formats
# INSTALL(DIRECTORY "${QT_PLUGINS_DIR}/imageformats" DESTINATION ${PLUGIN_DEST_DIR} COMPONENT Runtime)
# 
# # Platform plugins
# INSTALL(DIRECTORY "${QT_PLUGINS_DIR}/platforms" DESTINATION ${PLUGIN_DEST_DIR} COMPONENT Runtime)
# 
# # qtconf
# INSTALL(CODE "
# 	FILE(WRITE \"\${CMAKE_INSTALL_PREFIX}/${QTCONF_DEST_DIR}/qt.conf\" \"\")
# 	" COMPONENT Runtime)
# 
# 
# # Dirs to look for dependencies.
# SET(DIRS "${QT_LIBRARY_DIRS}")
# 
# INSTALL(CODE "
# 	file(GLOB_RECURSE QTPLUGINS
# 		\"\${CMAKE_INSTALL_PREFIX}/${PLUGIN_DEST_DIR}/plugins/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
# 	include(BundleUtilities)
# 	fixup_bundle(\"${APPS}\" \"\${QTPLUGINS}\" \"${DIRS}\")
# 	" COMPONENT Runtime)


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

include_directories(${PROJECT_BINARY_DIR}/include)

### translation stuff

file (GLOB TRANSLATIONS_FILES translations/*.ts)

option (UPDATE_TRANSLATIONS "Update source translation translations/*.ts files (WARNING: make clean will delete the source .ts files! Danger!)")
if (UPDATE_TRANSLATIONS)
  qt5_create_translation(QM_FILES ${FILES_TO_TRANSLATE} ${TRANSLATIONS_FILES})
else (UPDATE_TRANSLATIONS)
  qt5_add_translation(QM_FILES ${TRANSLATIONS_FILES})
endif (UPDATE_TRANSLATIONS) 

add_custom_target (translations_target DEPENDS ${QM_FILES})

install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/translations)
