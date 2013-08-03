project(libMultiMC)

set(CMAKE_AUTOMOC ON)

# Find Qt
find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Xml REQUIRED)

# Include Qt headers.
include_directories(${Qt5Base_INCLUDE_DIRS})
include_directories(${Qt5Network_INCLUDE_DIRS})

# Include utility library.
include_directories(${CMAKE_SOURCE_DIR}/libutil/include)

# Include settings library.
include_directories(${CMAKE_SOURCE_DIR}/libsettings/include)

SET(LIBINST_HEADERS
libmmc_config.h

# Instance Stuff
BaseInstance.h
LegacyInstance.h
OneSixInstance.h
InstanceFactory.h

# Versions
InstanceVersion.h
MinecraftVersion.h
OneSixVersion.h
VersionFactory.h

# Lists
lists/InstanceList.h
lists/InstVersionList.h
lists/MinecraftVersionList.h
lists/LwjglVersionList.h

# Tasks
tasks/Task.h
tasks/LoginTask.h
tasks/LoginResponse.h
tasks/UserInfo.h
tasks/GameUpdateTask.h

MinecraftProcess.h
)

SET(LIBINST_SOURCES
# Instance Stuff
BaseInstance.cpp
LegacyInstance.cpp
OneSixInstance.cpp
InstanceFactory.cpp

# Versions
InstanceVersion.cpp
MinecraftVersion.cpp
OneSixVersion.cpp
VersionFactory.cpp

# Lists
lists/InstanceList.cpp
lists/InstVersionList.cpp
lists/MinecraftVersionList.cpp
lists/LwjglVersionList.cpp

# Tasks
tasks/Task.cpp
tasks/LoginTask.cpp
tasks/GameUpdateTask.cpp
tasks/UserInfo.cpp
tasks/LoginResponse.cpp

MinecraftProcess.cpp
)

# Set the include dir path.
SET(LIBMULTIMC_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PARENT_SCOPE)

# Include self.
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR}/include)

add_definitions(-DLIBMULTIMC_LIBRARY)

add_library(backend SHARED ${LIBINST_SOURCES} ${LIBINST_HEADERS})
qt5_use_modules(backend Core Network Xml)
target_link_libraries(backend libUtil libSettings)


