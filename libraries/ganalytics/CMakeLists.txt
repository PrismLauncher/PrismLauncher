project(ganalytics)

find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Network)

set(ganalytics_SOURCES
src/ganalytics.cpp
src/ganalytics_worker.cpp
src/ganalytics_worker.h
include/ganalytics.h
)

add_library(ganalytics STATIC ${ganalytics_SOURCES})
target_link_libraries(ganalytics Qt5::Core Qt5::Gui Qt5::Network)
target_include_directories(ganalytics PUBLIC include)
target_link_libraries(ganalytics systeminfo)
