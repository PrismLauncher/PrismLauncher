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

# Base classes and infrastructure
InstanceVersion.h
MinecraftVersion.h
InstanceFactory.h
BaseUpdate.h
BaseInstance.h
BaseInstance_p.h
MinecraftProcess.h
Mod.h
ModList.h

# network stuffs
net/DownloadJob.h
net/JobQueue.h
net/NetWorker.h

# legacy instances
LegacyInstance.h
LegacyInstance_p.h
LegacyUpdate.h
LegacyForge.h

# 1.6 instances
OneSixAssets.h
OneSixInstance.h
OneSixInstance_p.h
OneSixUpdate.h
OneSixVersion.h
VersionFactory.h

# Nostalgia
NostalgiaInstance.h

# Lists
lists/InstanceList.h
lists/InstVersionList.h
lists/MinecraftVersionList.h
lists/LwjglVersionList.h
IconListModel.h

# Tasks
tasks/Task.h
tasks/LoginTask.h
)

SET(LIBINST_SOURCES
# Base classes and infrastructure
InstanceFactory.cpp
BaseUpdate.cpp
BaseInstance.cpp
MinecraftProcess.cpp
Mod.cpp
ModList.cpp

# network stuffs
net/NetWorker.cpp
net/DownloadJob.cpp

# legacy instances
LegacyInstance.cpp
LegacyUpdate.cpp
LegacyForge.cpp

# 1.6 instances
OneSixAssets.cpp
OneSixInstance.cpp
OneSixVersion.cpp
OneSixUpdate.cpp
VersionFactory.cpp

# Nostalgia
NostalgiaInstance.cpp

# Lists
lists/InstanceList.cpp
lists/InstVersionList.cpp
lists/MinecraftVersionList.cpp
lists/LwjglVersionList.cpp
IconListModel.cpp

# Tasks
tasks/Task.cpp
tasks/LoginTask.cpp
)

# Set the include dir path.
SET(LIBMULTIMC_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PARENT_SCOPE)

# Include self.
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR}/include)

add_definitions(-DLIBMULTIMC_LIBRARY)

add_library(backend SHARED ${LIBINST_SOURCES} ${LIBINST_HEADERS})
qt5_use_modules(backend Core Network Xml)
target_link_libraries(backend libUtil libSettings quazip)

