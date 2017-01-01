project(systeminfo)

find_package(Qt5Core)

set(systeminfo_SOURCES
include/sys.h
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
qt5_use_modules(systeminfo Core Gui Network)
target_include_directories(systeminfo PUBLIC include)

include (UnitTest)
add_unit_test(sys
	SOURCES src/sys_test.cpp
	LIBS systeminfo
)
