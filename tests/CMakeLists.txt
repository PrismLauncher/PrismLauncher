include(UnitTest)

add_unit_test(gradlespecifier
	SOURCES tst_gradlespecifier.cpp
	LIBS MultiMC_logic
	)

add_unit_test(userutils
	SOURCES tst_userutils.cpp
	LIBS MultiMC_logic
	)

add_unit_test(modutils
	SOURCES tst_modutils.cpp
	LIBS MultiMC_logic
	)

add_unit_test(inifile
	SOURCES tst_inifile.cpp
	LIBS MultiMC_logic
	)

add_unit_test(FileSystem
	SOURCES tst_FileSystem.cpp
	LIBS MultiMC_logic
	)

add_unit_test(Library
	SOURCES tst_Library.cpp
	LIBS MultiMC_logic
	)

add_unit_test(UpdateChecker
	SOURCES tst_UpdateChecker.cpp
	LIBS MultiMC_logic
	DATA data
	)

add_unit_test(DownloadTask
	SOURCES tst_DownloadTask.cpp
	LIBS MultiMC_logic
	DATA data_raw
	)

add_unit_test(filematchers
	SOURCES tst_filematchers.cpp
	LIBS MultiMC_logic
	)

add_unit_test(ModList
	SOURCES tst_ModList.cpp
	LIBS MultiMC_logic
	)

# add_unit_test(Resource
#	SOURCES tst_Resource.cpp
#	)

add_unit_test(GZip
	SOURCES tst_GZip.cpp
	LIBS MultiMC_logic
	)

add_unit_test(JavaVersion
	SOURCES tst_JavaVersion.cpp
	LIBS MultiMC_logic
	)

add_unit_test(ParseUtils
	SOURCES tst_ParseUtils.cpp
	LIBS MultiMC_logic
	)

add_unit_test(MojangVersionFormat
	SOURCES tst_MojangVersionFormat.cpp
	LIBS MultiMC_logic
	)

add_unit_test(WonkoIndex
	SOURCES tst_WonkoIndex.cpp
	LIBS MultiMC_logic
	)
