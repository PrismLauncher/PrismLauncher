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
qt5_use_modules(ganalytics Core Gui Network)
target_include_directories(ganalytics PUBLIC include)
target_link_libraries(ganalytics systeminfo)
