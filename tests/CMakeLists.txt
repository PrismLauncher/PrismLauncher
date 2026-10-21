project(tests)

create_test_sourcelist(SRCS main.cpp
    FileSystem_test.cpp GZip_test.cpp GradleSpecifier_test.cpp MojangVersionFormat_test.cpp Library_test.cpp
    ResourceFolderModel_test.cpp ResourcePackParse_test.cpp TexturePackParse_test.cpp DataPackParse_test.cpp
    ShaderPackParse_test.cpp WorldSaveParse_test.cpp ParseUtils_test.cpp Task_test.cpp INIFile_test.cpp
    JavaVersion_test.cpp Packwiz_test.cpp Index_test.cpp Version_test.cpp ModrinthUrl_test.cpp
    MetaComponentParse_test.cpp CatPack_test.cpp XmlLogs_test.cpp Commandline_test.cpp
)

qt_add_executable(Launcher_testrunner ${SRCS})
target_link_libraries(Launcher_testrunner PRIVATE Launcher_logic Qt${QT_VERSION_MAJOR}::Test)

function(add_launcher_test NAME)
    add_test(NAME ${NAME} COMMAND Launcher_testrunner "${NAME}_test")
endfunction()

add_launcher_test(FileSystem)
add_launcher_test(GZip)
add_launcher_test(GradleSpecifier)
add_launcher_test(MojangVersionFormat)
add_launcher_test(Library)
add_launcher_test(ResourceFolderModel)
add_launcher_test(ResourcePackParse)
add_launcher_test(TexturePackParse)
add_launcher_test(DataPackParse)
add_launcher_test(ShaderPackParse)
add_launcher_test(WorldSaveParse)
add_launcher_test(ParseUtils)
add_launcher_test(Task)
add_launcher_test(INIFile)
add_launcher_test(JavaVersion)
add_launcher_test(Packwiz)
add_launcher_test(Index)
add_launcher_test(Version)
add_launcher_test(ModrinthUrl)
add_launcher_test(MetaComponentParse)
add_launcher_test(CatPack)
add_launcher_test(XmlLogs)
add_launcher_test(Commandline)