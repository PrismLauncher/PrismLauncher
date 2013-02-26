project(libMultiMC)

set(CMAKE_AUTOMOC ON)

# Find Qt
find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)

# Include Qt headers.
include_directories(${Qt5Base_INCLUDE_DIRS})
include_directories(${Qt5Network_INCLUDE_DIRS})

# Include utility library.
include_directories(${CMAKE_SOURCE_DIR}/libutil/include)

# Include settings library.
include_directories(${CMAKE_SOURCE_DIR}/libsettings/include)

SET(LIBINST_HEADERS
include/libmmc_config.h


# Instance Stuff
include/instance.h
include/instancelist.h
include/instanceloader.h

include/instversion.h
include/instversionlist.h


# Plugin Stuff
include/pluginmanager.h
include/instancetypeinterface.h


# Tasks
include/task.h
include/logintask.h
include/gameupdatetask.h


# Login Data
include/userinfo.h
include/loginresponse.h


# Misc Data
include/version.h
include/appsettings.h
include/minecraftprocess.h
)

SET(LIBINST_SOURCES
# Instance Stuff
src/instance.cpp
src/instancelist.cpp
src/instanceloader.cpp

src/instversion.cpp
src/instversionlist.cpp


# Plugin Stuff
src/pluginmanager.cpp


# Tasks
src/task.cpp
src/logintask.cpp
src/gameupdatetask.cpp


# Login Data
src/userinfo.cpp
src/loginresponse.cpp


# Misc Data
src/version.cpp
src/appsettings.cpp
src/minecraftprocess.cpp
)

# Set the include dir path.
SET(LIBMULTIMC_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)

# Include self.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR}/include)

add_definitions(-DLIBMULTIMC_LIBRARY)

add_library(libMultiMC SHARED ${LIBINST_SOURCES} ${LIBINST_HEADERS})
qt5_use_modules(libMultiMC Core Network)
target_link_libraries(libMultiMC libUtil libSettings)
