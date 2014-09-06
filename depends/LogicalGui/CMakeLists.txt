project(LogicalGui)

# Set the include dir path.
set(LOGICALGUI_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PARENT_SCOPE)

add_library(LogicalGui STATIC LogicalGui.h)
qt5_use_modules(LogicalGui Core)
