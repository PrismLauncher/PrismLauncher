project(libGroupView)

set(CMAKE_AUTOMOC ON)

# Find Qt
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)

# Include Qt headers.
include_directories(${Qt5Base_INCLUDE_DIRS})

SET(LIBGROUPVIEW_HEADERS
include/groupview_config.h

# Public headers
include/categorizedsortfilterproxymodel.h
include/categorizedview.h
include/categorydrawer.h

# Private headers
src/categorizedsortfilterproxymodel_p.h
src/categorizedview_p.h
)

SET(LIBGROUPVIEW_SOURCES
src/categorizedsortfilterproxymodel.cpp
src/categorizedview.cpp
src/categorydrawer.cpp
)

# Set the include dir path.
SET(LIBGROUPVIEW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)

# Include self.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR}/include)

# Static link!
ADD_DEFINITIONS(-DLIBGROUPVIEW_STATIC)

add_definitions(-DLIBGROUPVIEW_LIBRARY)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(libGroupView STATIC ${LIBGROUPVIEW_SOURCES} ${LIBGROUPVIEW_HEADERS})
qt5_use_modules(libGroupView Core Widgets)
