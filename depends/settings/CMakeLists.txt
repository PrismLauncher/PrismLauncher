project(libSettings)

# Find Qt
find_package(Qt5Core REQUIRED)

# Include Qt headers.
include_directories(${Qt5Base_INCLUDE_DIRS})

include(UseCXX11)
include(Coverage)

set(LIBSETTINGS_SOURCES
	libsettings_config.h

	inifile.h
	inifile.cpp

	settingsobject.h
	settingsobject.cpp
	inisettingsobject.h
	inisettingsobject.cpp

	setting.h
	setting.cpp
	overridesetting.h
	overridesetting.cpp
)

# Set the include dir path.
set(LIBSETTINGS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PARENT_SCOPE)

# Static link!
add_definitions(-DLIBSETTINGS_STATIC)

add_definitions(-DLIBSETTINGS_LIBRARY)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(libSettings STATIC ${LIBSETTINGS_SOURCES})
qt5_use_modules(libSettings Core)
target_link_libraries(libSettings)
