project(LogicalGui)
add_library(LogicalGui STATIC LogicalGui.h)
target_include_directories(LogicalGui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
qt5_use_modules(LogicalGui Core)
