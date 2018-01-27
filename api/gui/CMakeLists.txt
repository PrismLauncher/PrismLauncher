project(MultiMC_gui LANGUAGES CXX)

set(GUI_SOURCES
	DesktopServices.h
	DesktopServices.cpp

	# Icons
	icons/MMCIcon.h
	icons/MMCIcon.cpp
	icons/IconList.h
	icons/IconList.cpp

	SkinUtils.cpp
	SkinUtils.h
)
################################ COMPILE ################################

add_library(MultiMC_gui SHARED ${GUI_SOURCES})
set_target_properties(MultiMC_gui PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN 1)

generate_export_header(MultiMC_gui)

# Link
target_link_libraries(MultiMC_gui MultiMC_iconfix MultiMC_logic)
qt5_use_modules(MultiMC_gui Gui)

# Mark and export headers
target_include_directories(MultiMC_gui PUBLIC "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")

# Install it
install(
	TARGETS MultiMC_gui
	RUNTIME DESTINATION ${LIBRARY_DEST_DIR}
	LIBRARY DESTINATION ${LIBRARY_DEST_DIR}
)