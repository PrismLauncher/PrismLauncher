project(libmmcutil)

# Find Qt
find_package(Qt5Core REQUIRED)

# Include Qt headers.
include_directories(${Qt5Base_INCLUDE_DIRS})
include_directories(${Qt5Network_INCLUDE_DIRS})

SET(LIBUTIL_HEADERS
include/libutil_config.h

include/apputils.h

include/pathutils.h
include/osutils.h

include/inifile.h

include/siglist.h
include/siglist_impl.h
)

SET(LIBUTIL_SOURCES
src/pathutils.cpp
src/osutils.cpp

src/inifile.cpp
)

# Set the include dir path.
SET(LIBMMCUTIL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)

add_definitions(-DLIBMMCUTIL_LIBRARY)

add_library(libmmcutil SHARED ${LIBUTIL_SOURCES} ${LIBUTIL_HEADERS})
qt5_use_modules(libmmcutil Core)
target_link_libraries(libmmcutil)
