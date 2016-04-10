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

# Tests BEGIN #

add_unit_test(gradlespecifier tst_gradlespecifier.cpp)
add_unit_test(userutils tst_userutils.cpp)
add_unit_test(modutils tst_modutils.cpp)
add_unit_test(inifile tst_inifile.cpp)
add_unit_test(FileSystem tst_FileSystem.cpp)
add_unit_test(Library tst_Library.cpp)
add_unit_test(UpdateChecker tst_UpdateChecker.cpp)
add_unit_test(DownloadTask tst_DownloadTask.cpp)
add_unit_test(filematchers tst_filematchers.cpp)
add_unit_test(ModList tst_ModList.cpp)
# add_unit_test(Resource tst_Resource.cpp)
add_unit_test(GZip tst_GZip.cpp)
add_unit_test(JavaVersion tst_JavaVersion.cpp)
add_unit_test(ParseUtils tst_ParseUtils.cpp)
add_unit_test(MojangVersionFormat tst_MojangVersionFormat.cpp)
add_unit_test(BaseWonkoEntityLocalLoadTask tst_BaseWonkoEntityLocalLoadTask.cpp)
add_unit_test(BaseWonkoEntityRemoteLoadTask tst_BaseWonkoEntityRemoteLoadTask.cpp)
add_unit_test(WonkoVersionList tst_WonkoVersionList.cpp)
add_unit_test(WonkoIndex tst_WonkoIndex.cpp)

# Tests END #


set(MultiMC_TEST_DATA_PATH "${CMAKE_CURRENT_BINARY_DIR}/data")
set(MultiMC_TEST_DATA_PATH_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/data")
set(MultiMC_TEST_DATA_PATH_SOURCE_RAW "${CMAKE_CURRENT_SOURCE_DIR}/data_raw")

if(UNIX)
	# on unix we get the third / from the filename
	set(MultiMC_TEST_DATA_URL "file://${MultiMC_TEST_DATA_PATH}")
else()
	# we don't on windows, so we have to add it ourselves
	set(MultiMC_TEST_DATA_URL "file:///${MultiMC_TEST_DATA_PATH}")
endif()

# clean and replace test data
add_custom_command(
	TARGET test_data
	COMMAND ${CMAKE_COMMAND} -DMultiMC_TEST_DATA_URL=${MultiMC_TEST_DATA_URL} -DMultiMC_TEST_DATA_PATH=${MultiMC_TEST_DATA_PATH} -DMultiMC_TEST_DATA_PATH_SOURCE=${MultiMC_TEST_DATA_PATH_SOURCE} -DMultiMC_TEST_DATA_PATH_SOURCE_RAW=${MultiMC_TEST_DATA_PATH_SOURCE_RAW} -P ${CMAKE_CURRENT_SOURCE_DIR}/copy_tests.cmake 
)

configure_file(test_config.h.in test_config.h @ONLY)
