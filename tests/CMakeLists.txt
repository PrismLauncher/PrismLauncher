project(tests)

qt_add_executable(Launcher_testrunner
    FileSystem_test.cpp GZip_test.cpp GradleSpecifier_test.cpp MojangVersionFormat_test.cpp Library_test.cpp
    ResourceFolderModel_test.cpp ResourcePackParse_test.cpp TexturePackParse_test.cpp DataPackParse_test.cpp
    ShaderPackParse_test.cpp WorldSaveParse_test.cpp ParseUtils_test.cpp Task_test.cpp INIFile_test.cpp
    JavaVersion_test.cpp Packwiz_test.cpp Index_test.cpp Version_test.cpp ModrinthUrl_test.cpp
    MetaComponentParse_test.cpp CatPack_test.cpp XmlLogs_test.cpp Commandline_test.cpp
    main.cpp
)
target_link_libraries(Launcher_testrunner PRIVATE Launcher_logic Qt${QT_VERSION_MAJOR}::Test)

add_test(NAME FileSystem COMMAND Launcher_testrunner FileSystemTest)
add_test(NAME GZip COMMAND Launcher_testrunner GZipTest)
add_test(NAME GradleSpecifier COMMAND Launcher_testrunner GradleSpecifierTest)
add_test(NAME MojangVersionFormat COMMAND Launcher_testrunner MojangVersionFormatTest)
add_test(NAME Library COMMAND Launcher_testrunner LibraryTest)
add_test(NAME ResourceFolderModel COMMAND Launcher_testrunner ResourceFolderModelTest)
add_test(NAME ResourcePackParse COMMAND Launcher_testrunner ResourcePackParseTest)
add_test(NAME TexturePackParse COMMAND Launcher_testrunner TexturePackParseTest)
add_test(NAME DataPackParse COMMAND Launcher_testrunner DataPackParseTest)
add_test(NAME ShaderPackParse COMMAND Launcher_testrunner ShaderPackParseTest)
add_test(NAME WorldSaveParse COMMAND Launcher_testrunner WorldSaveParseTest)
add_test(NAME ParseUtils COMMAND Launcher_testrunner ParseUtilsTest)
add_test(NAME Task COMMAND Launcher_testrunner TaskTest)
add_test(NAME INIFile COMMAND Launcher_testrunner IniFileTest)
add_test(NAME JavaVersion COMMAND Launcher_testrunner JavaVersionTest)
add_test(NAME Packwiz COMMAND Launcher_testrunner PackwizTest)
add_test(NAME Index COMMAND Launcher_testrunner IndexTest)
add_test(NAME Version COMMAND Launcher_testrunner VersionTest)
add_test(NAME ModrinthUrl COMMAND Launcher_testrunner ModrinthUrlTest)
add_test(NAME MetaComponentParse COMMAND Launcher_testrunner MetaComponentParseTest)
add_test(NAME CatPack COMMAND Launcher_testrunner CatPackTest)
add_test(NAME XmlLogs COMMAND Launcher_testrunner XmlLogParseTest)
add_test(NAME Commandline COMMAND Launcher_testrunner CommandlineTest)