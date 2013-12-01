project(updater)

cmake_minimum_required(VERSION 2.6)
enable_testing()
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

include_directories(depends)

if (WIN32)
    include_directories(depends/win32cpp/include)

    if(MSVC)
		# - Link the updater binary statically with the Visual C++ runtime
		#   so that the executable can function standalone.
		# - Enable PDB generation for release builds
		set(CMAKE_CXX_FLAGS_DEBUG "/MT")
		set(CMAKE_C_FLAGS_DEBUG "/MT")

		set(CMAKE_CXX_FLAGS_RELEASE "/MT /Zi /O2 /Ob2 /D NDEBUG")
		set(CMAKE_C_FLAGS_RELEASE "/MT /Zi /O2 /Ob2 /D NDEBUG")
		remove_definitions(-DUNICODE -D_UNICODE)
    endif()
else()
	# optimize for reduced code size
	set(CMAKE_CXX_FLAGS_RELEASE "-Os")
	set(CMAKE_C_FLAGS_RELEASE "-Os")
endif()

if (APPLE)
	# Build the updater as a dual 32/64bit binary.  If only one architecture
	# is required, removing the other architecture will reduce the size
	# of the updater binary
	set(CMAKE_OSX_ARCHITECTURES i386;x86_64)

	# Build the updater so that it works on OS X 10.5 and above.
	set(MIN_OSX_VERSION 10.5)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmacosx-version-min=${MIN_OSX_VERSION}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=${MIN_OSX_VERSION}")
endif()

add_subdirectory(src)
add_subdirectory(depends/AnyOption)
add_subdirectory(depends/tinyxml)

