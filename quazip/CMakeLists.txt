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

# Must be added to enable export macro
ADD_DEFINITIONS(-DQUAZIP_BUILD)

#qt5_wrap_cpp(MOC_SRCS ${PUBLIC_HEADERS})
#set(SRCS ${SRCS} ${MOC_SRCS})

set(CMAKE_POSITION_INDEPENDENT_CODE ON) 

add_library(quazip STATIC ${SRCS})
target_link_libraries(quazip ${ZLIB_LIBRARIES})

#install(FILES ${PUBLIC_HEADERS} DESTINATION include/quazip)
#install(TARGETS quazip LIBRARY DESTINATION ${LIB_DESTINATION} ARCHIVE DESTINATION ${LIB_DESTINATION} RUNTIME DESTINATION ${LIB_DESTINATION})
