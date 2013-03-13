project(libGroupView)

set(CMAKE_AUTOMOC ON)

# Find Qt
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)

# Include Qt headers.
include_directories(${Qt5Base_INCLUDE_DIRS})

SET(LIBGROUPVIEW_HEADERS
include/libgroupview_config.h

# Public headers
include/kcategorizedsortfilterproxymodel.h
include/kcategorizedview.h
include/kcategorydrawer.h

# Private headers
src/kcategorizedsortfilterproxymodel_p.h
src/kcategorizedview_p.h
)

SET(LIBGROUPVIEW_SOURCES
src/kcategorizedsortfilterproxymodel.cpp
src/kcategorizedview.cpp
src/kcategorydrawer.cpp
)

# Set the include dir path.
SET(LIBGROUPVIEW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)

# Include self.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR}/include)

add_definitions(-DLIBGROUPVIEW_LIBRARY)

add_library(libGroupView SHARED ${LIBGROUPVIEW_SOURCES} ${LIBGROUPVIEW_HEADERS})
qt5_use_modules(libGroupView Core Widgets)
