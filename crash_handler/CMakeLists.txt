set(CRASH_HANDLER_SOURCES
  src/CrashHandler.cpp
  include/crash_handler/CrashHandler.h

  src/CrashHandlerDialog.cpp
  include/crash_handler/CrashHandlerDialog.h
)

include(CheckIncludeFiles)
include(CheckSymbolExists)

check_symbol_exists(sigaltstack "signal.h" HAVE_SIGALTSTACK)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/crash_handler/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/crash_handler/config.h
)

add_library(CrashHandler STATIC  ${CRASH_HANDLER_SOURCES})
target_include_directories(CrashHandler PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
)
set_target_properties(CrashHandler PROPERTIES
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/src/ui"
)

target_link_libraries(CrashHandler
    PUBLIC cpptrace::cpptrace
    PRIVATE Launcher_rainbow
    PRIVATE BuildConfig
    PUBLIC Qt${QT_VERSION_MAJOR}::Widgets
    PUBLIC Qt${QT_VERSION_MAJOR}::Core
)


# Set basic compiler warning/error flags for all targets
get_property(Launcher_TARGETS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY
             BUILDSYSTEM_TARGETS)
foreach(target ${Launcher_TARGETS})
    message(STATUS "Enabling all warnings as errors for target '${target}'")
    if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_compile_options(${target} PRIVATE /W4 /WX /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic
                               -Werror)
    endif()
endforeach()
