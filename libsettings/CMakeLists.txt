project(libmmcsettings)

# Find Qt
find_package(Qt5Core REQUIRED)

# Include Qt headers.
include_directories(${Qt5Base_INCLUDE_DIRS})
include_directories(${Qt5Network_INCLUDE_DIRS})

# Include utils library headers.
include_directories(${CMAKE_SOURCE_DIR}/libutil/include)

SET(LIBSETTINGS_HEADERS
include/libsettings_config.h

include/settingsobject.h
include/setting.h

include/basicsettingsobject.h
)

SET(LIBSETTINGS_SOURCES
src/settingsobject.cpp
src/setting.cpp

src/basicsettingsobject.cpp
)

# Set the include dir path.
SET(LIBMMCSETTINGS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)

add_definitions(-DLIBMMCSETTINGS_LIBRARY)

add_library(libmmcsettings SHARED ${LIBSETTINGS_SOURCES} ${LIBSETTINGS_HEADERS})
qt5_use_modules(libmmcsettings Core)
target_link_libraries(libmmcsettings libmmcutil)
