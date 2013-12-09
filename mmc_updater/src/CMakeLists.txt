
add_subdirectory(tests)

find_package(Threads REQUIRED)
include(GenerateCppResourceFile)

set (UPDATER_SOURCES
	AppInfo.cpp
	AppInfo.h
	DirIterator.cpp
	DirIterator.h
	FileUtils.cpp
	FileUtils.h
	Log.cpp
	Log.h
	ProcessUtils.cpp
	ProcessUtils.h
	StandardDirs.cpp
	StandardDirs.h
	UpdateDialog.cpp
	UpdateInstaller.cpp
	UpdateInstaller.h
	UpdateScript.cpp
	UpdateScript.h
	UpdaterOptions.cpp
	UpdaterOptions.h
)

add_definitions(-DTIXML_USE_STL)

if (WIN32)
	set(UPDATER_SOURCES ${UPDATER_SOURCES} UpdateDialogWin32.cpp UpdateDialogWin32.h)
endif()

if (UNIX)
	set(UPDATER_SOURCES ${UPDATER_SOURCES} UpdateDialogAscii.cpp UpdateDialogAscii.h)
	add_definitions(-Wall -Werror -Wconversion)
if (APPLE)
	set(MAC_DOCK_ICON_CPP_FILE ${CMAKE_CURRENT_BINARY_DIR}/mac_dock_icon.cpp)
	set(MAC_INFO_PLIST_FILE ${CMAKE_CURRENT_BINARY_DIR}/mac_info_plist.cpp)
	generate_cpp_resource_file(resource_macdockicon ${CMAKE_CURRENT_SOURCE_DIR}/resources/mac.icns ${MAC_DOCK_ICON_CPP_FILE})
	generate_cpp_resource_file(resource_macplist ${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist ${MAC_INFO_PLIST_FILE})
	set(UPDATER_SOURCES ${UPDATER_SOURCES}
		MacBundle.h
		MacBundle.cpp
		StandardDirs.mm
		StlSymbolsLeopard.cpp
		UpdateDialogCocoa.mm
		UpdateDialogCocoa.h
		mac_dock_icon.cpp
		mac_info_plist.cpp
	)
else() # linuxes and other similar systems
	find_package(GTK2 REQUIRED gtk)
	include_directories(${GTK2_INCLUDE_DIRS})
	add_library(updatergtk SHARED UpdateDialogGtk.cpp UpdateDialogGtk.h)
	target_link_libraries(updatergtk ${GTK2_LIBRARIES})

	# embed the GTK helper library into the updater binary.
	# At runtime it will be extracted and loaded if the
	# GTK libraries are available
	get_property(GTK_UPDATER_LIB TARGET updatergtk PROPERTY LOCATION)
	set(GTK_BIN_CPP_FILE ${CMAKE_CURRENT_BINARY_DIR}/libupdatergtk.cpp)
	generate_cpp_resource_file(resource_updatergtk ${GTK_UPDATER_LIB} ${GTK_BIN_CPP_FILE})
	add_dependencies(resource_updatergtk updatergtk)

	set(UPDATER_SOURCES ${UPDATER_SOURCES} UpdateDialogGtkFactory.cpp UpdateDialogGtkFactory.h ${GTK_BIN_CPP_FILE})
endif()
endif()

add_library(updatershared STATIC ${UPDATER_SOURCES})

target_link_libraries(updatershared
  anyoption
  tinyxml
)

if (UNIX)
	if (APPLE)
		find_library(COCOA_LIBRARY Cocoa)
		find_library(SECURITY_LIBRARY Security)
		target_link_libraries(updatershared ${SECURITY_LIBRARY} ${COCOA_LIBRARY})
	else()
		add_dependencies(updatershared resource_updatergtk)
	endif()
	target_link_libraries(updatershared pthread dl)
endif()

if (WIN32)
	set(EXE_FLAGS WIN32 resources/updater.rc)
endif()

add_executable(updater ${EXE_FLAGS} main.cpp)

target_link_libraries(updater
  updatershared
)


#### Updater Executable ####
IF(WIN32)
INSTALL(TARGETS updater 
	BUNDLE DESTINATION . COMPONENT Runtime
	LIBRARY DESTINATION . COMPONENT Runtime
	RUNTIME DESTINATION . COMPONENT Runtime
)
ENDIF()
IF(UNIX)
IF(APPLE)
INSTALL(TARGETS updater 
	BUNDLE DESTINATION . COMPONENT Runtime
	RUNTIME DESTINATION MultiMC.app/Contents/MacOS COMPONENT Runtime
)
ELSE()
INSTALL(TARGETS updater 
	BUNDLE DESTINATION . COMPONENT Runtime
	RUNTIME DESTINATION bin COMPONENT Runtime
)
ENDIF()
ENDIF()

