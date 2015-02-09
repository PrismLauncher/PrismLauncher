project(classparser)

set(CMAKE_AUTOMOC ON)

######## Check endianness ########
include(TestBigEndian)
test_big_endian(BIGENDIAN)
if(${BIGENDIAN})
	add_definitions(-DMULTIMC_BIG_ENDIAN)
endif(${BIGENDIAN})

# Find Qt
find_package(Qt5Core REQUIRED)

# Include Qt headers.
include_directories(${Qt5Base_INCLUDE_DIRS})

set(CLASSPARSER_HEADERS
include/classparser_config.h

# Public headers
include/javautils.h

# Private headers
src/annotations.h
src/classfile.h
src/constants.h
src/errors.h
src/javaendian.h
src/membuffer.h
)

set(CLASSPARSER_SOURCES
src/javautils.cpp
src/annotations.cpp
)

# Set the include dir path.
set(LIBGROUPVIEW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)

# Include self.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR}/include)

add_definitions(-DCLASSPARSER_LIBRARY)

add_library(classparser SHARED ${CLASSPARSER_SOURCES} ${CLASSPARSER_HEADERS})
qt5_use_modules(classparser Core)
