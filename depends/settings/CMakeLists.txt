project(libSettings)

# Find Qt
find_package(Qt5Core REQUIRED)

# Include Qt headers.
include_directories(${Qt5Base_INCLUDE_DIRS})


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

IF(MultiMC_CODE_COVERAGE)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 --coverage")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --coverage")
	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 --coverage")
	SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -O0 --coverage")
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O0 --coverage")
ENDIF(MultiMC_CODE_COVERAGE)

add_library(libSettings STATIC ${LIBSETTINGS_SOURCES})
qt5_use_modules(libSettings Core)
target_link_libraries(libSettings)
