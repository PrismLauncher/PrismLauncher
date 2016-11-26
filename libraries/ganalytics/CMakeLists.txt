project(ganalytics)

find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Network)

set(ganalytics_SOURCES
src/ganalytics.cpp
src/ganalytics_worker.cpp
src/ganalytics_worker.h
include/sys.h
include/ganalytics.h
)

if (WIN32)
	list(APPEND ganalytics_SOURCES src/sys_win32.cpp)
elseif (UNIX)
	if(APPLE)
		list(APPEND ganalytics_SOURCES src/sys_apple.cpp)
	else()
		list(APPEND ganalytics_SOURCES src/sys_unix.cpp)
	endif()
endif()

add_library(ganalytics STATIC ${ganalytics_SOURCES})
qt5_use_modules(ganalytics Core Gui Network)
target_include_directories(ganalytics PUBLIC include)

include (UnitTest)
add_unit_test(sys
	SOURCES src/sys_test.cpp
	LIBS ganalytics
)
