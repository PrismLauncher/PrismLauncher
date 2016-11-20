project(ganalytics)

find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Network)

add_library(ganalytics STATIC ganalytics.cpp ganalytics.h)
qt5_use_modules(ganalytics Core Gui Network)
target_include_directories(ganalytics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(ganalytics PRIVATE -DQT_GUI_LIB)
