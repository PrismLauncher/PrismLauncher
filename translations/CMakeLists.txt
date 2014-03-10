set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

### translation stuff

file(GLOB TRANSLATION_FILES ${CMAKE_CURRENT_LIST_DIR}/*.ts)
set(FILES_TO_TRANSLATE_ABSOLUTE)
foreach(file ${FILES_TO_TRANSLATE})
    list(APPEND FILES_TO_TRANSLATE_ABSOLUTE "${CMAKE_SOURCE_DIR}/${file}")
endforeach()

qt5_create_translation(TRANSLATION_MESSAGES ${FILES_TO_TRANSLATE_ABSOLUTE} ${TRANSLATION_FILES})
qt5_add_translation(TRANSLATION_QM ${TRANSLATION_FILES})
add_custom_target(translations_update DEPENDS ${TRANSLATION_MESSAGES})
add_custom_target(translations DEPENDS ${TRANSLATION_QM})

IF(APPLE AND UNIX) ## OSX
      install(FILES ${TRANSLATION_QM} DESTINATION MultiMC.app/Contents/MacOS/translations)
ELSE()
      install(FILES ${TRANSLATION_QM} DESTINATION translations)
ENDIF()
