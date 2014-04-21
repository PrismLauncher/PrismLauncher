project(libUtil)

include(UseCXX11)
include(Coverage)

# Find Qt
find_package(Qt5Core REQUIRED)

# Include Qt headers.
include_directories(${Qt5Base_INCLUDE_DIRS})

set(LIBUTIL_SOURCES
	include/libutil_config.h

	include/pathutils.h
	src/pathutils.cpp

	include/osutils.h

	include/userutils.h
	src/userutils.cpp

	include/cmdutils.h
	src/cmdutils.cpp

	include/modutils.h
	src/modutils.cpp
)

# Set the include dir path.
set(LIBUTIL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)

# Static link!
add_definitions(-DLIBUTIL_STATIC)

add_definitions(-DLIBUTIL_LIBRARY)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(libUtil STATIC ${LIBUTIL_SOURCES})
qt5_use_modules(libUtil Core)
target_link_libraries(libUtil)
