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

gui/mainwindow.cpp
gui/modeditwindow.cpp
gui/settingsdialog.cpp
gui/newinstancedialog.cpp

util/pathutils.cpp
util/osutils.cpp

java/javautils.cpp
java/annotations.cpp
)

SET(MULTIMC_HEADERS
gui/mainwindow.h
gui/modeditwindow.h
gui/settingsdialog.h
gui/newinstancedialog.h

data/appsettings.h
data/inifile.h
data/instancebase.h
data/instancemodel.h
data/stdinstance.h
data/version.h

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
)

SET(MULTIMC5_UIS
gui/mainwindow.ui
gui/modeditwindow.ui
gui/settingsdialog.ui
gui/newinstancedialog.ui
)

IF(WIN32)
SET(MultiMC_LINK_ADDITIONAL_LIBS ${MultiMC_LINK_ADDITIONAL_LIBS}
Ws2_32)
ENDIF()

SET_SOURCE_FILES_PROPERTIES(resources/MultiMCLauncher.jar GENERATED)

QT5_WRAP_UI(MULTIMC_UI ${MULTIMC5_UIS})
QT5_ADD_RESOURCES(MULTIMC_QRC multimc.qrc)

add_executable(MultiMC ${MULTIMC_SOURCES} ${MULTIMC_HEADERS} ${MULTIMC_UI} ${MULTIMC_QRC})
qt5_use_modules(MultiMC Widgets)
target_link_libraries(MultiMC quazip patchlib ${MultiMC_LINK_ADDITIONAL_LIBS})
add_dependencies(MultiMC MultiMCLauncher)
install(TARGETS MultiMC RUNTIME DESTINATION .)

IF(WIN32)
IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
SET(D "d")
ELSE()
SET(D "")
ENDIF()

install(FILES "${Qt5_DIR}/plugins/platforms/qwindows${D}.dll" DESTINATION platforms)
install(FILES "${Qt5_DIR}/plugins/platforms/qminimal${D}.dll" DESTINATION platforms)
install(FILES "${Qt5_DIR}/bin/libEGL${D}.dll" DESTINATION .)

install(FILES "${Qt5_DIR}/plugins/imageformats/qsvg${D}.dll" DESTINATION imageformats)
install(FILES "${Qt5_DIR}/plugins/imageformats/qico${D}.dll" DESTINATION imageformats)
install(FILES "${Qt5_DIR}/plugins/imageformats/qgif${D}.dll" DESTINATION imageformats)
install(FILES "${Qt5_DIR}/plugins/imageformats/qjpeg${D}.dll" DESTINATION imageformats)

ELSEIF(UNIX)
IF (APPLE)
ELSE()

install(FILES "${Qt5_DIR}/plugins/platforms/libqlinuxfb.so" DESTINATION platforms)
install(FILES "${Qt5_DIR}/plugins/platforms/libqminimal.so" DESTINATION platforms)
install(FILES "${Qt5_DIR}/plugins/platforms/libqxcb.so" DESTINATION platforms)

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
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
set(CPACK_PACKAGE_FILE_NAME "MultiMC")
INCLUDE(CPack)
