project(stdinstance)

ADD_DEFINITIONS(-DQT_PLUGIN)

# Find Qt
find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)

# Include Qt headers.
include_directories(${Qt5Base_INCLUDE_DIRS})
include_directories(${Qt5Network_INCLUDE_DIRS})

# Include the Java library.
include_directories(${CMAKE_SOURCE_DIR}/java)

# Include utils library headers.
include_directories(${CMAKE_SOURCE_DIR}/libutil/include)

# Include settings library headers.
include_directories(${CMAKE_SOURCE_DIR}/libsettings/include)

# Include instance library headers. 
include_directories(${CMAKE_SOURCE_DIR}libinstance/include)

SET(STDINST_HEADERS
stdinstancetype.h
stdinstance.h
)

SET(STDINST_SOURCES
stdinstancetype.cpp
stdinstance.cpp
)

add_library(stdinstance SHARED ${STDINST_SOURCES} ${STDINST_HEADERS})

set_target_properties(stdinstance PROPERTIES PREFIX "")
set_target_properties(stdinstance PROPERTIES RUNTIME_OUTPUT_DIRECTORY "..")

qt5_use_modules(stdinstance Core Network)
target_link_libraries(stdinstance 
quazip
patchlib

# Link the util, settings, and instance libraries.
libmmcutil
libmmcsettings
libmmcinst
)
