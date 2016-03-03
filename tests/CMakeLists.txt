# run the unit tests with `make test`
find_package(Qt5Test)

add_custom_target(test_data)

unset(MultiMC_TESTS)
macro(add_unit_test name)
	unset(srcs)
	foreach(arg ${testname} ${ARGN})
		list(APPEND srcs ${CMAKE_CURRENT_SOURCE_DIR}/${arg})
	endforeach()
	if(WIN32)
		list(APPEND srcs ${CMAKE_CURRENT_SOURCE_DIR}/test.rc)
	endif()
	add_executable(tst_${name} ${srcs})
	add_dependencies(tst_${name} test_data)
	target_link_libraries(tst_${name} MultiMC_logic)
	qt5_use_modules(tst_${name} Test Core Network)
	list(APPEND MultiMC_TESTS tst_${name})
	add_test(NAME ${name} COMMAND tst_${name})
endmacro()

# Tests START #

add_unit_test(gradlespecifier tst_gradlespecifier.cpp)
add_unit_test(userutils tst_userutils.cpp)
add_unit_test(modutils tst_modutils.cpp)
add_unit_test(inifile tst_inifile.cpp)
add_unit_test(FileSystem tst_FileSystem.cpp)
add_unit_test(UpdateChecker tst_UpdateChecker.cpp)
add_unit_test(DownloadTask tst_DownloadTask.cpp)
add_unit_test(filematchers tst_filematchers.cpp)
add_unit_test(ModList tst_ModList.cpp)
add_unit_test(Resource tst_Resource.cpp)
add_unit_test(GZip tst_GZip.cpp)
add_unit_test(JavaVersion tst_JavaVersion.cpp)
add_unit_test(ParseUtils tst_ParseUtils.cpp)
add_unit_test(MojangVersionFormat tst_MojangVersionFormat.cpp)

# Tests END #

set(COVERAGE_SOURCE_DIRS
	${MMC_SRC}/logic/*
	${MMC_SRC}/logic/auth/*
	${MMC_SRC}/logic/auth/flows/*
	${MMC_SRC}/logic/lists/*
	${MMC_SRC}/logic/net/*
	${MMC_SRC}/logic/tasks/*
	${MMC_SRC}/application/*
	${MMC_SRC}/application/dialogs/*
	${MMC_SRC}/application/widgets/*
	${MMC_SRC}/depends/settings/include/*
	${MMC_SRC}/depends/settings/src/*
	${MMC_SRC}/depends/util/include/*
	${MMC_SRC}/depends/util/src/*
)

if(MultiMC_CODE_COVERAGE)
	unset(MultiMC_RUN_TESTS)
	unset(MultiMC_TEST_COVERAGE_FILES)

	foreach(test ${MultiMC_TESTS})
		add_custom_target(MultiMC_RUN_TEST_${test}
			COMMAND lcov -d ${CMAKE_CURRENT_BINARY_DIR} -z -q # clean test
				&& lcov -d ${MMC_BIN} -z -q # clean common
				&& lcov -d ${MMC_BIN}/depends/settings/CMakeFiles/libSettings.dir -z -q # clean settings
				&& lcov -d ${MMC_BIN}/depends/utils/CMakeFiles/libUtil.dir -z -q # clean utils
				&& ${MMC_BIN}/${test} -o coverage_${test}.out,xml # run test
				&& lcov -q --checksum -b ${MMC_SRC} -d ${CMAKE_CURRENT_BINARY_DIR} -c -o coverage_${test}.info # generate for test
				&& lcov -q --checksum -b ${MMC_SRC} -d ${MMC_BIN} -c -o coverage_common.info # generate for common
				&& lcov -q --checksum -b ${MMC_SRC} -d ${MMC_BIN}/depends/settings/CMakeFiles/libSettings.dir -c -o coverage_settings.info # generate for settings
				&& lcov -q --checksum -b ${MMC_SRC} -d ${MMC_BIN}/depends/util/CMakeFiles/libUtil.dir -c -o coverage_utils.info # generate for utils
				&& lcov -q --checksum -b ${MMC_SRC} -d .
					-a coverage_${test}.info -a coverage_common.info -a coverage_settings.info -a coverage_utils.info
					-o coverage_${test}-combined.info # combine test and common
				&& lcov -q --checksum -b ${MMC_SRC} --list-full-path --extract coverage_${test}-combined.info ${COVERAGE_SOURCE_DIRS} -o coverage_${test}-stripped.info # strip
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			VERBATIM
			DEPENDS ${test}
			COMMENT "Running ${test}..."
		)
		list(APPEND MultiMC_TEST_COVERAGE_FILES coverage_${test}-stripped.info)
		list(APPEND MultiMC_RUN_TESTS MultiMC_RUN_TEST_${test})
	endforeach(test)

	add_custom_target(MultiMC_GENERATE_COVERAGE
		DEPENDS ${MultiMC_RUN_TESTS}
		COMMENT "Generating coverage files..."
	)
	add_custom_target(MultiMC_GENERATE_COVERAGE_HTML
		COMMAND genhtml -t "MultiMC 5 Test Coverage" --num-spaces 4 --demangle-cpp --legend -o ${MMC_SRC}/html/coverage ${MultiMC_TEST_COVERAGE_FILES}
		DEPENDS MultiMC_GENERATE_COVERAGE
		COMMENT "Generating test coverage html..."
	)
	add_custom_target(MultiMC_RUN_TESTS DEPENDS MultiMC_GENERATE_COVERAGE_HTML)
endif(MultiMC_CODE_COVERAGE)


set(MultiMC_TEST_DATA_PATH "${CMAKE_CURRENT_BINARY_DIR}/data")
set(MultiMC_TEST_DATA_PATH_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/data")
set(MultiMC_TEST_DATA_PATH_SOURCE_RAW "${CMAKE_CURRENT_SOURCE_DIR}/data_raw")

# clean and replace test data
add_custom_command(
	TARGET test_data
	COMMAND ${CMAKE_COMMAND} -E remove_directory ${MultiMC_TEST_DATA_PATH}
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${MultiMC_TEST_DATA_PATH_SOURCE} ${MultiMC_TEST_DATA_PATH}
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${MultiMC_TEST_DATA_PATH_SOURCE_RAW} ${MultiMC_TEST_DATA_PATH}
)

message("${CMAKE_CURRENT_BINARY_DIR}/data")
if(UNIX)
	# on unix we get the third / from the filename
	set(MultiMC_TEST_DATA_PATH "file://${MultiMC_TEST_DATA_PATH}")
else()
	# we don't on windows, so we have to add it ourselves
	set(MultiMC_TEST_DATA_PATH "file:///${MultiMC_TEST_DATA_PATH}")
endif()
configure_file(test_config.h.in test_config.h @ONLY)
