project(libUtil)

######## Set compiler flags ########
IF(APPLE)
	# assume clang 4.1.0+, add C++0x/C++11 stuff
	message(STATUS "Using APPLE CMAKE_CXX_FLAGS")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
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

IF(MultiMC_CODE_COVERAGE)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 --coverage")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --coverage")
	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 --coverage")
	SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -O0 --coverage")
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O0 --coverage")
ENDIF(MultiMC_CODE_COVERAGE)

add_library(libUtil STATIC ${LIBUTIL_SOURCES})
qt5_use_modules(libUtil Core)
target_link_libraries(libUtil)
