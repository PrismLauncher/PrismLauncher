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
include/classparser.h

# Private headers
src/annotations.h
src/classfile.h
src/constants.h
src/errors.h
src/javaendian.h
src/membuffer.h
)

set(CLASSPARSER_SOURCES
src/classparser.cpp
src/annotations.cpp
)

add_definitions(-DCLASSPARSER_LIBRARY)

add_library(MultiMC_classparser STATIC ${CLASSPARSER_SOURCES} ${CLASSPARSER_HEADERS})
target_include_directories(MultiMC_classparser PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(MultiMC_classparser MultiMC_quazip)
qt5_use_modules(MultiMC_classparser Core)
