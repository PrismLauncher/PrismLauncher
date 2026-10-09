project(tests)

ecm_add_test(
    FileSystem_test.cpp GZip_test.cpp GradleSpecifier_test.cpp MojangVersionFormat_test.cpp Library_test.cpp
    ResourceFolderModel_test.cpp ResourcePackParse_test.cpp TexturePackParse_test.cpp DataPackParse_test.cpp
    ShaderPackParse_test.cpp WorldSaveParse_test.cpp ParseUtils_test.cpp Task_test.cpp INIFile_test.cpp
    JavaVersion_test.cpp Packwiz_test.cpp Index_test.cpp Version_test.cpp ModrinthUrl_test.cpp
    MetaComponentParse_test.cpp CatPack_test.cpp XmlLogs_test.cpp Commandline_test.cpp
    main.cpp
    LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME LauncherTests
)