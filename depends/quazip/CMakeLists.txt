project(quazip)

# use QtCreator's QTDIR var
IF(DEFINED ENV{QTDIR})
	SET(Qt5_DIR $ENV{QTDIR})
ENDIF()

# Find ZLIB for quazip
# Use system zlib on unix and Qt ZLIB on Windows
IF(UNIX)
	find_package(ZLIB REQUIRED)
ELSE(UNIX)
	SET(ZLIB_INCLUDE_DIRS "${Qt5_DIR}/include/QtZlib" CACHE PATH "Path to ZLIB headers of Qt")
	SET(ZLIB_LIBRARIES "")
	IF(NOT EXISTS "${ZLIB_INCLUDE_DIRS}/zlib.h")
		MESSAGE("Please specify a valid zlib include dir")
	ENDIF(NOT EXISTS "${ZLIB_INCLUDE_DIRS}/zlib.h")
ENDIF(UNIX)

# set all include directories for in and out of source builds
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${ZLIB_INCLUDE_DIRS}
)

# include with QT_USE selected library parts
# INCLUDE(${QT_USE_FILE})

file(GLOB SRCS "*.c" "*.cpp")
file(GLOB PUBLIC_HEADERS "*.h")

# Static link!
ADD_DEFINITIONS(-DQUAZIP_STATIC)

#qt5_wrap_cpp(MOC_SRCS ${PUBLIC_HEADERS})
#set(SRCS ${SRCS} ${MOC_SRCS})

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(quazip STATIC ${SRCS})
QT5_USE_MODULES(quazip Core)
target_link_libraries(quazip ${ZLIB_LIBRARIES})

#install(FILES ${PUBLIC_HEADERS} DESTINATION include/quazip)
#install(TARGETS quazip LIBRARY DESTINATION ${LIB_DESTINATION} ARCHIVE DESTINATION ${LIB_DESTINATION} RUNTIME DESTINATION ${LIB_DESTINATION})
