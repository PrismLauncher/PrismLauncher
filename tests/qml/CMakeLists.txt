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
