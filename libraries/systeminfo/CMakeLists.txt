project(systeminfo)

find_package(Qt5Core)

set(systeminfo_SOURCES
include/sys.h
include/distroutils.h
src/distroutils.cpp
)

if (WIN32)
    list(APPEND systeminfo_SOURCES src/sys_win32.cpp)
elseif (UNIX)
    if(APPLE)
        list(APPEND systeminfo_SOURCES src/sys_apple.cpp)
    else()
        list(APPEND systeminfo_SOURCES src/sys_unix.cpp)
    endif()
endif()

add_library(systeminfo STATIC ${systeminfo_SOURCES})
target_link_libraries(systeminfo Qt5::Core Qt5::Gui Qt5::Network)
target_include_directories(systeminfo PUBLIC include)

ecm_add_test(src/sys_test.cpp LINK_LIBRARIES systeminfo Qt5::Test TEST_NAME sys)
