project(libSettings)

# Find Qt
find_package(Qt5Core REQUIRED)

# Include Qt headers.
include_directories(${Qt5Base_INCLUDE_DIRS})
include_directories(${Qt5Network_INCLUDE_DIRS})

SET(LIBSETTINGS_HEADERS
include/libsettings_config.h

include/inifile.h

include/settingsobject.h
include/setting.h
include/overridesetting.h

include/basicsettingsobject.h
include/inisettingsobject.h

include/keyring.h
)

SET(LIBSETTINGS_HEADERS_PRIVATE
src/stubkeyring.h
)

SET(LIBSETTINGS_SOURCES
src/inifile.cpp

src/settingsobject.cpp
src/setting.cpp
src/overridesetting.cpp

src/basicsettingsobject.cpp
src/inisettingsobject.cpp

src/keyring.cpp
src/stubkeyring.cpp
)

# Set the include dir path.
SET(LIBSETTINGS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)
include_directories(${LIBSETTINGS_INCLUDE_DIR})

# Static link!
ADD_DEFINITIONS(-DLIBSETTINGS_STATIC)

add_definitions(-DLIBSETTINGS_LIBRARY)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

IF(MultiMC_CODE_COVERAGE)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 --coverage")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --coverage")
	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 --coverage")
	SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -O0 --coverage")
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O0 --coverage")
ENDIF(MultiMC_CODE_COVERAGE)

add_library(libSettings STATIC ${LIBSETTINGS_SOURCES} ${LIBSETTINGS_HEADERS} ${LIBSETTINGS_HEADERS_PRIVATE})
qt5_use_modules(libSettings Core)
target_link_libraries(libSettings)
