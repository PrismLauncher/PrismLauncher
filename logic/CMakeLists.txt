project(libMultiMC)

set(CMAKE_AUTOMOC ON)

# Find Qt
find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Xml REQUIRED)

# Include Qt headers.
include_directories(${Qt5Base_INCLUDE_DIRS})
include_directories(${Qt5Network_INCLUDE_DIRS})

# Include utility library.
include_directories(${CMAKE_SOURCE_DIR}/libutil/include)

# Include settings library.
include_directories(${CMAKE_SOURCE_DIR}/libsettings/include)

SET(LIBINST_HEADERS

)


