project(stdinstance)

ADD_DEFINITIONS(-DQT_PLUGIN)

# Find Qt
find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)

# Include Qt headers.
include_directories(${Qt5Base_INCLUDE_DIRS})
include_directories(${Qt5Network_INCLUDE_DIRS})

# Include MultiMC's headers.
include_directories(../../)

SET(STDINST_HEADERS
stdinstplugin.h
stdinstancetype.h
stdinstance.h
)

SET(STDINST_SOURCES
stdinstplugin.cpp
stdinstancetype.cpp
stdinstance.cpp
)

add_library(stdinstance SHARED ${STDINST_SOURCES} ${STDINST_HEADERS})
set_target_properties(stdinstance PROPERTIES PREFIX "")
qt5_use_modules(stdinstance Core Network)
target_link_libraries(stdinstance quazip patchlib)
