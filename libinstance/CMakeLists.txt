project(libmmcinst)

set(CMAKE_AUTOMOC ON)

# Find Qt
find_package(Qt5Core REQUIRED)

# Include Qt headers.
include_directories(${Qt5Base_INCLUDE_DIRS})
include_directories(${Qt5Network_INCLUDE_DIRS})

# Include utility library.
include_directories(${CMAKE_SOURCE_DIR}/libutil/include)

# Include utility library.
include_directories(${CMAKE_SOURCE_DIR}/libsettings/include)

SET(LIBINST_HEADERS
include/libinstance_config.h

include/instancetypeinterface.h

include/instance.h
include/instancelist.h
include/instanceloader.h

include/instversion.h
include/instversionlist.h
)

SET(LIBINST_SOURCES
src/instance.cpp
src/instancelist.cpp
src/instanceloader.cpp

src/instversion.cpp
src/instversionlist.cpp
)

# Set the include dir path.
SET(LIBMMCINST_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)

add_definitions(-DLIBMMCINST_LIBRARY)

add_library(libmmcinst SHARED ${LIBINST_SOURCES} ${LIBINST_HEADERS})
qt5_use_modules(libmmcinst Core)
target_link_libraries(libmmcinst libmmcutil libmmcsettings)
