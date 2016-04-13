find_package(Qt5Test REQUIRED)

set(TEST_RESOURCE_PATH ${CMAKE_CURRENT_LIST_DIR})

message(${TEST_RESOURCE_PATH})

function(add_unit_test name)
	set(options "")
	set(oneValueArgs DATA)
	set(multiValueArgs SOURCES LIBS QT)

	cmake_parse_arguments(OPT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

	if(WIN32)
		add_executable(${name}_test ${OPT_SOURCES} ${TEST_RESOURCE_PATH}/UnitTest/test.rc)
	else()
		add_executable(${name}_test ${OPT_SOURCES})
	endif()

	if(NOT "${OPT_DATA}" STREQUAL "")
		set(TEST_DATA_PATH "${CMAKE_CURRENT_BINARY_DIR}/data")
		set(TEST_DATA_PATH_SRC "${CMAKE_CURRENT_SOURCE_DIR}/${OPT_DATA}")
		message("From ${TEST_DATA_PATH_SRC} to ${TEST_DATA_PATH}")
		string(REGEX REPLACE "[/\\:]" "_" DATA_TARGET_NAME "${TEST_DATA_PATH_SRC}")
		if(UNIX)
			# on unix we get the third / from the filename
			set(TEST_DATA_URL "file://${TEST_DATA_PATH}")
		else()
			# we don't on windows, so we have to add it ourselves
			set(TEST_DATA_URL "file:///${TEST_DATA_PATH}")
		endif()
		if(NOT TARGET "${DATA_TARGET_NAME}")
			add_custom_target(${DATA_TARGET_NAME})
			add_dependencies(${name}_test ${DATA_TARGET_NAME})
			add_custom_command(
				TARGET ${DATA_TARGET_NAME}
				COMMAND ${CMAKE_COMMAND} "-DTEST_DATA_URL=${TEST_DATA_URL}" -DSOURCE=${TEST_DATA_PATH_SRC} -DDESTINATION=${TEST_DATA_PATH} -P ${TEST_RESOURCE_PATH}/UnitTest/generate_test_data.cmake
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			)
		endif()
	endif()

	target_link_libraries(${name}_test ${OPT_LIBS})
	qt5_use_modules(${name}_test Test ${OPT_QT})

	target_include_directories(${name}_test PRIVATE "${TEST_RESOURCE_PATH}/UnitTest/")

	add_test(NAME ${name} COMMAND ${name}_test)
endfunction()
