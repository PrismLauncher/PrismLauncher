find_program(LINT_EXECUTABLE NAMES qmllint qmllint-qt5 HINTS ${QT_BINS_DIR} NO_DEFAULT_PATH)

if(NOT (LINT_EXECUTABLE STREQUAL "LINT_EXECUTABLE-NOTFOUND"))
  file(GLOB_RECURSE QML_FILES "../../launcher/ui/*.qml")
  message(DEBUG "QML files found for testing: ${QML_FILES}")

  add_test(
    NAME qmllint
    COMMAND ${LINT_EXECUTABLE} ${QML_FILES}
    COMMAND_EXPAND_LISTS
  )
else()
  message(WARNING "No 'qmllint' executable found. The corresponding test will be skipped.")
endif()

ecm_add_test(Theming_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME QmlThemingBasic)
ecm_add_test(CustomThemeLoading_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME QmlCustomThemeLoading)

# NOTE: Qt has some leak issues, especially on their wayland plugin. Since this test uses a GUI,
#       it ends up triggering those issues, and the test fails. This prevents it from failing.
set_tests_properties(QmlCustomThemeLoading PROPERTIES ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0")
