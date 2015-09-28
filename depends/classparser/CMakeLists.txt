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
# Public headers
include/classparser_config.h
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

add_definitions(-DCLASSPARSER_LIBRARY)

add_library(classparser SHARED ${CLASSPARSER_SOURCES} ${CLASSPARSER_HEADERS})
target_include_directories(classparser PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
qt5_use_modules(classparser Core)
