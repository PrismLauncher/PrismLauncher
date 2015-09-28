project(MultiMC_util)

include(Coverage)

# Find Qt
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)

set(LIBUTIL_SOURCES
	src/pathutils.cpp
	src/userutils.cpp
	src/cmdutils.cpp
	src/modutils.cpp
	include/cmdutils.h
	include/modutils.h
	include/osutils.h
	include/pathutils.h
	include/userutils.h
)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(MultiMC_util STATIC ${LIBUTIL_SOURCES})

qt5_use_modules(MultiMC_util Core Gui)
generate_export_header(MultiMC_util)
target_include_directories(MultiMC_util PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_property(TARGET MultiMC_util PROPERTY CXX_STANDARD 14)
