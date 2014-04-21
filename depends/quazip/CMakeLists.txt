project(quazip)

# Find ZLIB for quazip
# Use system zlib on unix and Qt ZLIB on Windows
if(UNIX)
	find_package(ZLIB REQUIRED)
else(UNIX)
	get_filename_component(ZLIB_FOUND_DIR "${Qt5Core_DIR}/../../../include/QtZlib" ABSOLUTE)
	set(ZLIB_INCLUDE_DIRS ${ZLIB_FOUND_DIR} CACHE PATH "Path to ZLIB headers of Qt")
	set(ZLIB_LIBRARIES "")
	if(NOT EXISTS "${ZLIB_INCLUDE_DIRS}/zlib.h")
		message("Please specify a valid zlib include dir")
	endif(NOT EXISTS "${ZLIB_INCLUDE_DIRS}/zlib.h")
endif(UNIX)

# set all include directories for in and out of source builds
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${ZLIB_INCLUDE_DIRS}
)

file(GLOB SRCS "*.c" "*.cpp")
file(GLOB PUBLIC_HEADERS "*.h")

# Static link!
add_definitions(-DQUAZIP_STATIC)

add_library(quazip STATIC ${SRCS})
qt5_use_modules(quazip Core)
target_link_libraries(quazip ${ZLIB_LIBRARIES})
