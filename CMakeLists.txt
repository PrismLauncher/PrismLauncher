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

IF(UNIX)
  # assume GCC, add C++0x/C++11 stuff
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
ELSEIF(MINGW)
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
data/instancebase.cpp
data/instancemodel.cpp
data/stdinstance.cpp
data/version.cpp
data/userinfo.cpp
data/loginresponse.cpp

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
data/instancebase.h
data/instancemodel.h
data/stdinstance.h
data/version.h
data/userinfo.h
data/loginresponse.h

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

IF(WIN32)
SET(MultiMC_LINK_ADDITIONAL_LIBS ${MultiMC_LINK_ADDITIONAL_LIBS}
Ws2_32)
ENDIF()

SET_SOURCE_FILES_PROPERTIES(resources/MultiMCLauncher.jar GENERATED)

QT5_WRAP_UI(MULTIMC_UI ${MULTIMC5_UIS})
QT5_ADD_RESOURCES(MULTIMC_QRC multimc.qrc)

add_executable(MultiMC ${MULTIMC_SOURCES} ${MULTIMC_HEADERS} ${MULTIMC_UI} ${MULTIMC_QRC})
qt5_use_modules(MultiMC Widgets Network)
target_link_libraries(MultiMC quazip patchlib ${MultiMC_LINK_ADDITIONAL_LIBS})
add_dependencies(MultiMC MultiMCLauncher)

IF (WIN32)
install(TARGETS MultiMC RUNTIME DESTINATION .)
ELSE()
install(TARGETS MultiMC RUNTIME DESTINATION bin)
ENDIF()



# Extra libs and files to package.

IF(WIN32)
SET(LIB_INSTALL_PREFIX .)
SET(LIB_INSTALL_PREFIX_ABS ${CMAKE_INSTALL_PREFIX})
ELSE()
SET(LIB_INSTALL_PREFIX lib)
SET(LIB_INSTALL_PREFIX_ABS ${CMAKE_INSTALL_PREFIX}/lib)
ENDIF()


# Image format plugins.
SET(IMAGE_FORMAT_PLUGINS svg ico gif jpeg)

INCLUDE(GetPrerequisites)

# Includes DLL dependencies for the given file. Does not include installed system DLLs. Recursive.
MACRO(INCLUDE_DLL_DEPS DLL_FILE DEST)
GET_PREREQUISITES(${DLL_FILE} DLL_PREREQS 1 1 "" "")

MESSAGE(STATUS "Installing ${DLL_FILE} and its prerequisites.")
INSTALL(FILES ${DLL_FILE} DESTINATION ${DEST})

FOREACH(PREREQ ${DLL_PREREQS})
	GET_FILENAME_COMPONENT(PREREQ_NAME "${PREREQ}" NAME)
	GET_FILENAME_COMPONENT(PREREQ_ACTUAL "${PREREQ}" REALPATH)
	IF(WIN32)
	SET(PREREQ_ACTUAL "${Qt5_DIR}/bin/${PREREQ}")
	ENDIF()
	
	MESSAGE(STATUS "Adding install prerequisite for ${DLL_FILE}: ${PREREQ_NAME}")
	
	INSTALL(FILES ${PREREQ_ACTUAL} RENAME ${PREREQ_NAME} DESTINATION ${LIB_INSTALL_PREFIX})
ENDFOREACH()
ENDMACRO()

MACRO(INSTALL_SYMLINK_DEST LINK_FILENAME DEST)
GET_FILENAME_COMPONENT(DEST_NAME "${LINK_FILENAME}" NAME)
GET_FILENAME_COMPONENT(DEST_ACTUAL "${LINK_FILENAME}" REALPATH)

INSTALL(FILES "${DEST_ACTUAL}" RENAME "${DEST_NAME}" DESTINATION "${DEST}")
ENDMACRO()


IF(WIN32)

# Windows

IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
SET(D "d")
ELSE()
SET(D "")
ENDIF()

# Install platform plugins.
SET(PLATFORM_PLUGINS windows minimal)
FOREACH(PLATFORM_PLUGIN ${PLATFORM_PLUGINS})
	INCLUDE_DLL_DEPS("${Qt5_DIR}/plugins/platforms/q${PLATFORM_PLUGIN}${D}.dll" platforms)
ENDFOREACH()

# Install image format plugins.
FOREACH(IMGFMT_PLUGIN ${IMAGE_FORMAT_PLUGINS})
	INCLUDE_DLL_DEPS("${Qt5_DIR}/plugins/imageformats/q${IMGFMT_PLUGIN}${D}.dll" imageformats)
ENDFOREACH()

ELSEIF(UNIX)
IF (APPLE)

# OS X
# TODO: OS X packaging support

ELSE()

# Linux

# Install platform plugins.
SET(PLATFORM_PLUGINS linuxfb xcb minimal)
FOREACH(PLATFORM_PLUGIN ${PLATFORM_PLUGINS})
	INCLUDE_DLL_DEPS("${Qt5_DIR}/plugins/platforms/libq${PLATFORM_PLUGIN}.so" platforms)
ENDFOREACH()

# Install image format plugins.
FOREACH(IMGFMT_PLUGIN ${IMAGE_FORMAT_PLUGINS})
	INCLUDE_DLL_DEPS("${Qt5_DIR}/plugins/imageformats/libq${IMGFMT_PLUGIN}.so" imageformats)
ENDFOREACH()

# This stuff *should* be added automatically by the INCLUDE_ALL_DLL_DEPS macro. Include them manually just in case.
# Install ICU libs.
SET(ICU_LIBS data i18n io le lx test tu uc)
FOREACH(ICU_LIB ${ICU_LIBS})
	INSTALL_SYMLINK_DEST("/usr/lib/libicu${ICU_LIB}.so.48" ${LIB_INSTALL_PREFIX})
ENDFOREACH()

INSTALL_SYMLINK_DEST("/usr/lib/libxcb-render-util.so.0" ${LIB_INSTALL_PREFIX})

# Install additional libs.
#INSTALL(FILES "${Qt5_DIR}/lib/libQt5DBus.so.5.0.1" DESTINATION lib RENAME libQt5DBus.so.5)

# Install the start script.
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/package/linux/MultiMC" DESTINATION .)

ENDIF()
ENDIF()


GET_TARGET_PROPERTY(BINARY_LOCATION MultiMC LOCATION)
CONFIGURE_FILE(
   "${CMAKE_CURRENT_SOURCE_DIR}/dependencies.cmake.in"
   "${CMAKE_CURRENT_BINARY_DIR}/dependencies.cmake"
   @ONLY
   )
INSTALL(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/dependencies.cmake")


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
