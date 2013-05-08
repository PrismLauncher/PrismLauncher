project(libUtil)

######## Set compiler flags ########
IF(APPLE)
	# assume clang 4.1.0+, add C++0x/C++11 stuff
	message(STATUS "Using APPLE CMAKE_CXX_FLAGS")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -stdlib=libc++")
ELSEIF(UNIX)
	# assume GCC, add C++0x/C++11 stuff
	MESSAGE(STATUS "Using UNIX CMAKE_CXX_FLAGS")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
ELSEIF(MINGW)
	MESSAGE(STATUS "Using MINGW CMAKE_CXX_FLAGS")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x")
ENDIF()


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
include/userutils.h
include/cmdutils.h
include/netutils.h
)

SET(LIBUTIL_SOURCES
src/pathutils.cpp
src/osutils.cpp
src/userutils.cpp
src/cmdutils.cpp
src/netutils.cpp
)

# Set the include dir path.
SET(LIBUTIL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)

add_definitions(-DLIBUTIL_LIBRARY)

add_library(libUtil SHARED ${LIBUTIL_SOURCES} ${LIBUTIL_HEADERS})
qt5_use_modules(libUtil Core)
target_link_libraries(libUtil)
