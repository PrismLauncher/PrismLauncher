# - Use Module for Java
# This file provides functions for Java. It is assumed that FindJava.cmake
# has already been loaded.  See FindJava.cmake for information on how to
# load Java into your CMake project.
#
# add_jar(TARGET_NAME SRC1 SRC2 .. SRCN RCS1 RCS2 .. RCSN)
#
# This command creates a <TARGET_NAME>.jar. It compiles the given source
# files (SRC) and adds the given resource files (RCS) to the jar file.
# If only resource files are given then just a jar file is created.
#
# Additional instructions:
#   To add compile flags to the target you can set these flags with
#   the following variable:
#
#       set(CMAKE_JAVA_COMPILE_FLAGS -nowarn)
#
#   To add a path or a jar file to the class path you can do this
#   with the CMAKE_JAVA_INCLUDE_PATH variable.
#
#       set(CMAKE_JAVA_INCLUDE_PATH /usr/share/java/shibboleet.jar)
#
#   To use a different output name for the target you can set it with:
#
#       set(CMAKE_JAVA_TARGET_OUTPUT_NAME shibboleet.jar)
#       add_jar(foobar foobar.java)
#
#   To use a different output directory than CMAKE_CURRENT_BINARY_DIR
#   you can set it with:
#
#       set(CMAKE_JAVA_TARGET_OUTPUT_DIR ${PROJECT_BINARY_DIR}/bin)
#
#   To define an entry point in your jar you can set it with:
#
#       set(CMAKE_JAVA_JAR_ENTRY_POINT com/examples/MyProject/Main)
#
#   To add a VERSION to the target output name you can set it using
#   CMAKE_JAVA_TARGET_VERSION. This will create a jar file with the name
#   shibboleet-1.0.0.jar and will create a symlink shibboleet.jar
#   pointing to the jar with the version information.
#
#       set(CMAKE_JAVA_TARGET_VERSION 1.2.0)
#       add_jar(shibboleet shibbotleet.java)
#
#    If the target is a JNI library, utilize the following commands to
#    create a JNI symbolic link:
#
#       set(CMAKE_JNI_TARGET TRUE)
#       set(CMAKE_JAVA_TARGET_VERSION 1.2.0)
#       add_jar(shibboleet shibbotleet.java)
#       install_jar(shibboleet ${LIB_INSTALL_DIR}/shibboleet)
#       install_jni_symlink(shibboleet ${JAVA_LIB_INSTALL_DIR})
#
#    If a single target needs to produce more than one jar from its
#    java source code, to prevent the accumulation of duplicate class
#    files in subsequent jars, set/reset CMAKE_JAR_CLASSES_PREFIX prior
#    to calling the add_jar() function:
#
#       set(CMAKE_JAR_CLASSES_PREFIX com/redhat/foo)
#       add_jar(foo foo.java)
#
#       set(CMAKE_JAR_CLASSES_PREFIX com/redhat/bar)
#       add_jar(bar bar.java)
#
# Target Properties:
#   The add_jar() functions sets some target properties. You can get these
#   properties with the
#      get_property(TARGET <target_name> PROPERTY <propery_name>)
#   command.
#
#   INSTALL_FILES      The files which should be installed. This is used by
#                      install_jar().
#   JNI_SYMLINK        The JNI symlink which should be installed.
#                      This is used by install_jni_symlink().
#   JAR_FILE           The location of the jar file so that you can include
#                      it.
#   CLASS_DIR          The directory where the class files can be found. For
#                      example to use them with javah.
#
# find_jar(<VAR>
#          name | NAMES name1 [name2 ...]
#          [PATHS path1 [path2 ... ENV var]]
#          [VERSIONS version1 [version2]]
#          [DOC "cache documentation string"]
#         )
#
# This command is used to find a full path to the named jar. A cache
# entry named by <VAR> is created to stor the result of this command. If
# the full path to a jar is found the result is stored in the variable
# and the search will not repeated unless the variable is cleared. If
# nothing is found, the result will be <VAR>-NOTFOUND, and the search
# will be attempted again next time find_jar is invoked with the same
# variable.
# The name of the full path to a file that is searched for is specified
# by the names listed after NAMES argument. Additional search locations
# can be specified after the PATHS argument. If you require special a
# version of a jar file you can specify it with the VERSIONS argument.
# The argument after DOC will be used for the documentation string in
# the cache.
#
# install_jar(TARGET_NAME DESTINATION)
#
# This command installs the TARGET_NAME files to the given DESTINATION.
# It should be called in the same scope as add_jar() or it will fail.
#
# install_jni_symlink(TARGET_NAME DESTINATION)
#
# This command installs the TARGET_NAME JNI symlinks to the given
# DESTINATION. It should be called in the same scope as add_jar()
# or it will fail.
#
# create_javadoc(<VAR>
#                PACKAGES pkg1 [pkg2 ...]
#                [SOURCEPATH <sourcepath>]
#                [CLASSPATH <classpath>]
#                [INSTALLPATH <install path>]
#                [DOCTITLE "the documentation title"]
#                [WINDOWTITLE "the title of the document"]
#                [AUTHOR TRUE|FALSE]
#                [USE TRUE|FALSE]
#                [VERSION TRUE|FALSE]
#               )
#
# Create java documentation based on files or packages. For more
# details please read the javadoc manpage.
#
# There are two main signatures for create_javadoc. The first
# signature works with package names on a path with source files:
#
#   Example:
#   create_javadoc(my_example_doc
#     PACKAGES com.exmaple.foo com.example.bar
#     SOURCEPATH "${CMAKE_CURRENT_SOURCE_DIR}"
#     CLASSPATH ${CMAKE_JAVA_INCLUDE_PATH}
#     WINDOWTITLE "My example"
#     DOCTITLE "<h1>My example</h1>"
#     AUTHOR TRUE
#     USE TRUE
#     VERSION TRUE
#   )
#
# The second signature for create_javadoc works on a given list of
# files.
#
#   create_javadoc(<VAR>
#                  FILES file1 [file2 ...]
#                  [CLASSPATH <classpath>]
#                  [INSTALLPATH <install path>]
#                  [DOCTITLE "the documentation title"]
#                  [WINDOWTITLE "the title of the document"]
#                  [AUTHOR TRUE|FALSE]
#                  [USE TRUE|FALSE]
#                  [VERSION TRUE|FALSE]
#                 )
#
# Example:
#   create_javadoc(my_example_doc
#     FILES ${example_SRCS}
#     CLASSPATH ${CMAKE_JAVA_INCLUDE_PATH}
#     WINDOWTITLE "My example"
#     DOCTITLE "<h1>My example</h1>"
#     AUTHOR TRUE
#     USE TRUE
#     VERSION TRUE
#   )
#
# Both signatures share most of the options. These options are the
# same as what you can find in the javadoc manpage. Please look at
# the manpage for CLASSPATH, DOCTITLE, WINDOWTITLE, AUTHOR, USE and
# VERSION.
#
# The documentation will be by default installed to
#
#   ${CMAKE_INSTALL_PREFIX}/share/javadoc/<VAR>
#
# if you don't set the INSTALLPATH.
#

#=============================================================================
# Copyright 2010-2011 Andreas schneider <asn@redhat.com>
# Copyright 2010 Ben Boeckel <ben.boeckel@kitware.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

function (__java_copy_file src dest comment)
    add_custom_command(
        OUTPUT  ${dest}
        COMMAND cmake -E copy_if_different
        ARGS    ${src}
                ${dest}
        DEPENDS ${src}
        COMMENT ${comment})
endfunction (__java_copy_file src dest comment)

# define helper scripts
set(_JAVA_CLASS_FILELIST_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/UseJavaClassFilelist.cmake)
set(_JAVA_SYMLINK_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/UseJavaSymlinks.cmake)

function(add_jar _TARGET_NAME)
    set(_JAVA_SOURCE_FILES ${ARGN})

    if (NOT DEFINED CMAKE_JAVA_TARGET_OUTPUT_DIR)
      set(CMAKE_JAVA_TARGET_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
    endif(NOT DEFINED CMAKE_JAVA_TARGET_OUTPUT_DIR)

    if (CMAKE_JAVA_JAR_ENTRY_POINT)
      set(_ENTRY_POINT_OPTION e)
      set(_ENTRY_POINT_VALUE ${CMAKE_JAVA_JAR_ENTRY_POINT})
    endif (CMAKE_JAVA_JAR_ENTRY_POINT)

    if (LIBRARY_OUTPUT_PATH)
        set(CMAKE_JAVA_LIBRARY_OUTPUT_PATH ${LIBRARY_OUTPUT_PATH})
    else (LIBRARY_OUTPUT_PATH)
        set(CMAKE_JAVA_LIBRARY_OUTPUT_PATH ${CMAKE_JAVA_TARGET_OUTPUT_DIR})
    endif (LIBRARY_OUTPUT_PATH)

    set(CMAKE_JAVA_INCLUDE_PATH
        ${CMAKE_JAVA_INCLUDE_PATH}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_JAVA_OBJECT_OUTPUT_PATH}
        ${CMAKE_JAVA_LIBRARY_OUTPUT_PATH}
    )

    if (WIN32 AND NOT CYGWIN AND NOT CMAKE_CROSSCOMPILING)
        set(CMAKE_JAVA_INCLUDE_FLAG_SEP ";")
    else ()
        set(CMAKE_JAVA_INCLUDE_FLAG_SEP ":")
    endif()

    foreach (JAVA_INCLUDE_DIR ${CMAKE_JAVA_INCLUDE_PATH})
       set(CMAKE_JAVA_INCLUDE_PATH_FINAL "${CMAKE_JAVA_INCLUDE_PATH_FINAL}${CMAKE_JAVA_INCLUDE_FLAG_SEP}${JAVA_INCLUDE_DIR}")
    endforeach(JAVA_INCLUDE_DIR)

    set(CMAKE_JAVA_CLASS_OUTPUT_PATH "${CMAKE_JAVA_TARGET_OUTPUT_DIR}${CMAKE_FILES_DIRECTORY}/${_TARGET_NAME}.dir")

    set(_JAVA_TARGET_OUTPUT_NAME "${_TARGET_NAME}.jar")
    if (CMAKE_JAVA_TARGET_OUTPUT_NAME AND CMAKE_JAVA_TARGET_VERSION)
        set(_JAVA_TARGET_OUTPUT_NAME "${CMAKE_JAVA_TARGET_OUTPUT_NAME}-${CMAKE_JAVA_TARGET_VERSION}.jar")
        set(_JAVA_TARGET_OUTPUT_LINK "${CMAKE_JAVA_TARGET_OUTPUT_NAME}.jar")
    elseif (CMAKE_JAVA_TARGET_VERSION)
        set(_JAVA_TARGET_OUTPUT_NAME "${_TARGET_NAME}-${CMAKE_JAVA_TARGET_VERSION}.jar")
        set(_JAVA_TARGET_OUTPUT_LINK "${_TARGET_NAME}.jar")
    elseif (CMAKE_JAVA_TARGET_OUTPUT_NAME)
        set(_JAVA_TARGET_OUTPUT_NAME "${CMAKE_JAVA_TARGET_OUTPUT_NAME}.jar")
    endif (CMAKE_JAVA_TARGET_OUTPUT_NAME AND CMAKE_JAVA_TARGET_VERSION)
    # reset
    set(CMAKE_JAVA_TARGET_OUTPUT_NAME)

    set(_JAVA_CLASS_FILES)
    set(_JAVA_COMPILE_FILES)
    set(_JAVA_DEPENDS)
    set(_JAVA_RESOURCE_FILES)
    foreach(_JAVA_SOURCE_FILE ${_JAVA_SOURCE_FILES})
        get_filename_component(_JAVA_EXT ${_JAVA_SOURCE_FILE} EXT)
        get_filename_component(_JAVA_FILE ${_JAVA_SOURCE_FILE} NAME_WE)
        get_filename_component(_JAVA_PATH ${_JAVA_SOURCE_FILE} PATH)
        get_filename_component(_JAVA_FULL ${_JAVA_SOURCE_FILE} ABSOLUTE)

        file(RELATIVE_PATH _JAVA_REL_BINARY_PATH ${CMAKE_JAVA_TARGET_OUTPUT_DIR} ${_JAVA_FULL})
        file(RELATIVE_PATH _JAVA_REL_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${_JAVA_FULL})
        string(LENGTH ${_JAVA_REL_BINARY_PATH} _BIN_LEN)
        string(LENGTH ${_JAVA_REL_SOURCE_PATH} _SRC_LEN)
        if (${_BIN_LEN} LESS ${_SRC_LEN})
            set(_JAVA_REL_PATH ${_JAVA_REL_BINARY_PATH})
        else (${_BIN_LEN} LESS ${_SRC_LEN})
            set(_JAVA_REL_PATH ${_JAVA_REL_SOURCE_PATH})
        endif (${_BIN_LEN} LESS ${_SRC_LEN})
        get_filename_component(_JAVA_REL_PATH ${_JAVA_REL_PATH} PATH)

        if (_JAVA_EXT MATCHES ".java")
            list(APPEND _JAVA_COMPILE_FILES ${_JAVA_SOURCE_FILE})
            set(_JAVA_CLASS_FILE "${CMAKE_JAVA_CLASS_OUTPUT_PATH}/${_JAVA_REL_PATH}/${_JAVA_FILE}.class")
            set(_JAVA_CLASS_FILES ${_JAVA_CLASS_FILES} ${_JAVA_CLASS_FILE})

        elseif (_JAVA_EXT MATCHES ".jar"
                OR _JAVA_EXT MATCHES ".war"
                OR _JAVA_EXT MATCHES ".ear"
                OR _JAVA_EXT MATCHES ".sar")
            list(APPEND CMAKE_JAVA_INCLUDE_PATH ${_JAVA_SOURCE_FILE})

        elseif (_JAVA_EXT STREQUAL "")
            list(APPEND CMAKE_JAVA_INCLUDE_PATH ${JAVA_JAR_TARGET_${_JAVA_SOURCE_FILE}} ${JAVA_JAR_TARGET_${_JAVA_SOURCE_FILE}_CLASSPATH})
            list(APPEND _JAVA_DEPENDS ${JAVA_JAR_TARGET_${_JAVA_SOURCE_FILE}})

        else (_JAVA_EXT MATCHES ".java")
            __java_copy_file(${CMAKE_CURRENT_SOURCE_DIR}/${_JAVA_SOURCE_FILE}
                             ${CMAKE_JAVA_CLASS_OUTPUT_PATH}/${_JAVA_SOURCE_FILE}
                             "Copying ${_JAVA_SOURCE_FILE} to the build directory")
            list(APPEND _JAVA_RESOURCE_FILES ${_JAVA_SOURCE_FILE})
        endif (_JAVA_EXT MATCHES ".java")
    endforeach(_JAVA_SOURCE_FILE)

    # create an empty java_class_filelist
    if (NOT EXISTS ${CMAKE_JAVA_CLASS_OUTPUT_PATH}/java_class_filelist)
        file(WRITE ${CMAKE_JAVA_CLASS_OUTPUT_PATH}/java_class_filelist "")
    endif()

    if (_JAVA_COMPILE_FILES)
        # Compile the java files and create a list of class files
        add_custom_command(
            # NOTE: this command generates an artificial dependency file
            OUTPUT ${CMAKE_JAVA_CLASS_OUTPUT_PATH}/java_compiled_${_TARGET_NAME}
            COMMAND ${Java_JAVAC_EXECUTABLE}
                ${CMAKE_JAVA_COMPILE_FLAGS}
                -classpath "${CMAKE_JAVA_INCLUDE_PATH_FINAL}"
                -d ${CMAKE_JAVA_CLASS_OUTPUT_PATH}
                ${_JAVA_COMPILE_FILES}
            COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_JAVA_CLASS_OUTPUT_PATH}/java_compiled_${_TARGET_NAME}
            DEPENDS ${_JAVA_COMPILE_FILES}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Building Java objects for ${_TARGET_NAME}.jar"
        )
        add_custom_command(
            OUTPUT ${CMAKE_JAVA_CLASS_OUTPUT_PATH}/java_class_filelist
            COMMAND ${CMAKE_COMMAND}
                -DCMAKE_JAVA_CLASS_OUTPUT_PATH=${CMAKE_JAVA_CLASS_OUTPUT_PATH}
                -DCMAKE_JAR_CLASSES_PREFIX="${CMAKE_JAR_CLASSES_PREFIX}"
                -P ${_JAVA_CLASS_FILELIST_SCRIPT}
            DEPENDS ${CMAKE_JAVA_CLASS_OUTPUT_PATH}/java_compiled_${_TARGET_NAME}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif (_JAVA_COMPILE_FILES)

    # create the jar file
    set(_JAVA_JAR_OUTPUT_PATH
      ${CMAKE_JAVA_TARGET_OUTPUT_DIR}/${_JAVA_TARGET_OUTPUT_NAME})
    if (CMAKE_JNI_TARGET)
        add_custom_command(
            OUTPUT ${_JAVA_JAR_OUTPUT_PATH}
            COMMAND ${Java_JAR_EXECUTABLE}
                -cf${_ENTRY_POINT_OPTION} ${_JAVA_JAR_OUTPUT_PATH} ${_ENTRY_POINT_VALUE}
                ${_JAVA_RESOURCE_FILES} @java_class_filelist
            COMMAND ${CMAKE_COMMAND}
                -D_JAVA_TARGET_DIR=${CMAKE_JAVA_TARGET_OUTPUT_DIR}
                -D_JAVA_TARGET_OUTPUT_NAME=${_JAVA_TARGET_OUTPUT_NAME}
                -D_JAVA_TARGET_OUTPUT_LINK=${_JAVA_TARGET_OUTPUT_LINK}
                -P ${_JAVA_SYMLINK_SCRIPT}
            COMMAND ${CMAKE_COMMAND}
                -D_JAVA_TARGET_DIR=${CMAKE_JAVA_TARGET_OUTPUT_DIR}
                -D_JAVA_TARGET_OUTPUT_NAME=${_JAVA_JAR_OUTPUT_PATH}
                -D_JAVA_TARGET_OUTPUT_LINK=${_JAVA_TARGET_OUTPUT_LINK}
                -P ${_JAVA_SYMLINK_SCRIPT}
            DEPENDS ${_JAVA_RESOURCE_FILES} ${_JAVA_DEPENDS} ${CMAKE_JAVA_CLASS_OUTPUT_PATH}/java_class_filelist
            WORKING_DIRECTORY ${CMAKE_JAVA_CLASS_OUTPUT_PATH}
            COMMENT "Creating Java archive ${_JAVA_TARGET_OUTPUT_NAME}"
        )
    else ()
        add_custom_command(
            OUTPUT ${_JAVA_JAR_OUTPUT_PATH}
            COMMAND ${Java_JAR_EXECUTABLE}
                -cf${_ENTRY_POINT_OPTION} ${_JAVA_JAR_OUTPUT_PATH} ${_ENTRY_POINT_VALUE}
                ${_JAVA_RESOURCE_FILES} @java_class_filelist
            COMMAND ${CMAKE_COMMAND}
                -D_JAVA_TARGET_DIR=${CMAKE_JAVA_TARGET_OUTPUT_DIR}
                -D_JAVA_TARGET_OUTPUT_NAME=${_JAVA_TARGET_OUTPUT_NAME}
                -D_JAVA_TARGET_OUTPUT_LINK=${_JAVA_TARGET_OUTPUT_LINK}
                -P ${_JAVA_SYMLINK_SCRIPT}
            WORKING_DIRECTORY ${CMAKE_JAVA_CLASS_OUTPUT_PATH}
            DEPENDS ${_JAVA_RESOURCE_FILES} ${_JAVA_DEPENDS} ${CMAKE_JAVA_CLASS_OUTPUT_PATH}/java_class_filelist
            COMMENT "Creating Java archive ${_JAVA_TARGET_OUTPUT_NAME}"
        )
    endif (CMAKE_JNI_TARGET)

    # Add the target and make sure we have the latest resource files.
    add_custom_target(${_TARGET_NAME} ALL DEPENDS ${_JAVA_JAR_OUTPUT_PATH})

    set_property(
        TARGET
            ${_TARGET_NAME}
        PROPERTY
            INSTALL_FILES
                ${_JAVA_JAR_OUTPUT_PATH}
    )

    if (_JAVA_TARGET_OUTPUT_LINK)
        set_property(
            TARGET
                ${_TARGET_NAME}
            PROPERTY
                INSTALL_FILES
                    ${_JAVA_JAR_OUTPUT_PATH}
                    ${CMAKE_JAVA_TARGET_OUTPUT_DIR}/${_JAVA_TARGET_OUTPUT_LINK}
        )

        if (CMAKE_JNI_TARGET)
            set_property(
                TARGET
                    ${_TARGET_NAME}
                PROPERTY
                    JNI_SYMLINK
                        ${CMAKE_JAVA_TARGET_OUTPUT_DIR}/${_JAVA_TARGET_OUTPUT_LINK}
            )
        endif (CMAKE_JNI_TARGET)
    endif (_JAVA_TARGET_OUTPUT_LINK)

    set_property(
        TARGET
            ${_TARGET_NAME}
        PROPERTY
            JAR_FILE
                ${_JAVA_JAR_OUTPUT_PATH}
    )

    set_property(
        TARGET
            ${_TARGET_NAME}
        PROPERTY
            CLASSDIR
                ${CMAKE_JAVA_CLASS_OUTPUT_PATH}
    )

endfunction(add_jar)

function(INSTALL_JAR _TARGET_NAME _DESTINATION)
    get_property(__FILES
        TARGET
            ${_TARGET_NAME}
        PROPERTY
            INSTALL_FILES
    )

    if (__FILES)
        install(
            FILES
                ${__FILES}
            DESTINATION
                ${_DESTINATION}
        )
    else (__FILES)
        message(SEND_ERROR "The target ${_TARGET_NAME} is not known in this scope.")
    endif (__FILES)
endfunction(INSTALL_JAR _TARGET_NAME _DESTINATION)

function(INSTALL_JNI_SYMLINK _TARGET_NAME _DESTINATION)
    get_property(__SYMLINK
        TARGET
            ${_TARGET_NAME}
        PROPERTY
            JNI_SYMLINK
    )

    if (__SYMLINK)
        install(
            FILES
                ${__SYMLINK}
            DESTINATION
                ${_DESTINATION}
        )
    else (__SYMLINK)
        message(SEND_ERROR "The target ${_TARGET_NAME} is not known in this scope.")
    endif (__SYMLINK)
endfunction(INSTALL_JNI_SYMLINK _TARGET_NAME _DESTINATION)

function (find_jar VARIABLE)
    set(_jar_names)
    set(_jar_files)
    set(_jar_versions)
    set(_jar_paths
        /usr/share/java/
        /usr/local/share/java/
        ${Java_JAR_PATHS})
    set(_jar_doc "NOTSET")

    set(_state "name")

    foreach (arg ${ARGN})
        if (${_state} STREQUAL "name")
            if (${arg} STREQUAL "VERSIONS")
                set(_state "versions")
            elseif (${arg} STREQUAL "NAMES")
                set(_state "names")
            elseif (${arg} STREQUAL "PATHS")
                set(_state "paths")
            elseif (${arg} STREQUAL "DOC")
                set(_state "doc")
            else (${arg} STREQUAL "NAMES")
                set(_jar_names ${arg})
                if (_jar_doc STREQUAL "NOTSET")
                    set(_jar_doc "Finding ${arg} jar")
                endif (_jar_doc STREQUAL "NOTSET")
            endif (${arg} STREQUAL "VERSIONS")
        elseif (${_state} STREQUAL "versions")
            if (${arg} STREQUAL "NAMES")
                set(_state "names")
            elseif (${arg} STREQUAL "PATHS")
                set(_state "paths")
            elseif (${arg} STREQUAL "DOC")
                set(_state "doc")
            else (${arg} STREQUAL "NAMES")
                set(_jar_versions ${_jar_versions} ${arg})
            endif (${arg} STREQUAL "NAMES")
        elseif (${_state} STREQUAL "names")
            if (${arg} STREQUAL "VERSIONS")
                set(_state "versions")
            elseif (${arg} STREQUAL "PATHS")
                set(_state "paths")
            elseif (${arg} STREQUAL "DOC")
                set(_state "doc")
            else (${arg} STREQUAL "VERSIONS")
                set(_jar_names ${_jar_names} ${arg})
                if (_jar_doc STREQUAL "NOTSET")
                    set(_jar_doc "Finding ${arg} jar")
                endif (_jar_doc STREQUAL "NOTSET")
            endif (${arg} STREQUAL "VERSIONS")
        elseif (${_state} STREQUAL "paths")
            if (${arg} STREQUAL "VERSIONS")
                set(_state "versions")
            elseif (${arg} STREQUAL "NAMES")
                set(_state "names")
            elseif (${arg} STREQUAL "DOC")
                set(_state "doc")
            else (${arg} STREQUAL "VERSIONS")
                set(_jar_paths ${_jar_paths} ${arg})
            endif (${arg} STREQUAL "VERSIONS")
        elseif (${_state} STREQUAL "doc")
            if (${arg} STREQUAL "VERSIONS")
                set(_state "versions")
            elseif (${arg} STREQUAL "NAMES")
                set(_state "names")
            elseif (${arg} STREQUAL "PATHS")
                set(_state "paths")
            else (${arg} STREQUAL "VERSIONS")
                set(_jar_doc ${arg})
            endif (${arg} STREQUAL "VERSIONS")
        endif (${_state} STREQUAL "name")
    endforeach (arg ${ARGN})

    if (NOT _jar_names)
        message(FATAL_ERROR "find_jar: No name to search for given")
    endif (NOT _jar_names)

    foreach (jar_name ${_jar_names})
        foreach (version ${_jar_versions})
            set(_jar_files ${_jar_files} ${jar_name}-${version}.jar)
        endforeach (version ${_jar_versions})
        set(_jar_files ${_jar_files} ${jar_name}.jar)
    endforeach (jar_name ${_jar_names})

    find_file(${VARIABLE}
        NAMES   ${_jar_files}
        PATHS   ${_jar_paths}
        DOC     ${_jar_doc}
        NO_DEFAULT_PATH)
endfunction (find_jar VARIABLE)

function(create_javadoc _target)
    set(_javadoc_packages)
    set(_javadoc_files)
    set(_javadoc_sourcepath)
    set(_javadoc_classpath)
    set(_javadoc_installpath "${CMAKE_INSTALL_PREFIX}/share/javadoc")
    set(_javadoc_doctitle)
    set(_javadoc_windowtitle)
    set(_javadoc_author FALSE)
    set(_javadoc_version FALSE)
    set(_javadoc_use FALSE)

    set(_state "package")

    foreach (arg ${ARGN})
        if (${_state} STREQUAL "package")
            if (${arg} STREQUAL "PACKAGES")
                set(_state "packages")
            elseif (${arg} STREQUAL "FILES")
                set(_state "files")
            elseif (${arg} STREQUAL "SOURCEPATH")
                set(_state "sourcepath")
            elseif (${arg} STREQUAL "CLASSPATH")
                set(_state "classpath")
            elseif (${arg} STREQUAL "INSTALLPATH")
                set(_state "installpath")
            elseif (${arg} STREQUAL "DOCTITLE")
                set(_state "doctitle")
            elseif (${arg} STREQUAL "WINDOWTITLE")
                set(_state "windowtitle")
            elseif (${arg} STREQUAL "AUTHOR")
                set(_state "author")
            elseif (${arg} STREQUAL "USE")
                set(_state "use")
            elseif (${arg} STREQUAL "VERSION")
                set(_state "version")
            else ()
                set(_javadoc_packages ${arg})
                set(_state "packages")
            endif ()
        elseif (${_state} STREQUAL "packages")
            if (${arg} STREQUAL "FILES")
                set(_state "files")
            elseif (${arg} STREQUAL "SOURCEPATH")
                set(_state "sourcepath")
            elseif (${arg} STREQUAL "CLASSPATH")
                set(_state "classpath")
            elseif (${arg} STREQUAL "INSTALLPATH")
                set(_state "installpath")
            elseif (${arg} STREQUAL "DOCTITLE")
                set(_state "doctitle")
            elseif (${arg} STREQUAL "WINDOWTITLE")
                set(_state "windowtitle")
            elseif (${arg} STREQUAL "AUTHOR")
                set(_state "author")
            elseif (${arg} STREQUAL "USE")
                set(_state "use")
            elseif (${arg} STREQUAL "VERSION")
                set(_state "version")
            else ()
                list(APPEND _javadoc_packages ${arg})
            endif ()
        elseif (${_state} STREQUAL "files")
            if (${arg} STREQUAL "PACKAGES")
                set(_state "packages")
            elseif (${arg} STREQUAL "SOURCEPATH")
                set(_state "sourcepath")
            elseif (${arg} STREQUAL "CLASSPATH")
                set(_state "classpath")
            elseif (${arg} STREQUAL "INSTALLPATH")
                set(_state "installpath")
            elseif (${arg} STREQUAL "DOCTITLE")
                set(_state "doctitle")
            elseif (${arg} STREQUAL "WINDOWTITLE")
                set(_state "windowtitle")
            elseif (${arg} STREQUAL "AUTHOR")
                set(_state "author")
            elseif (${arg} STREQUAL "USE")
                set(_state "use")
            elseif (${arg} STREQUAL "VERSION")
                set(_state "version")
            else ()
                list(APPEND _javadoc_files ${arg})
            endif ()
        elseif (${_state} STREQUAL "sourcepath")
            if (${arg} STREQUAL "PACKAGES")
                set(_state "packages")
            elseif (${arg} STREQUAL "FILES")
                set(_state "files")
            elseif (${arg} STREQUAL "CLASSPATH")
                set(_state "classpath")
            elseif (${arg} STREQUAL "INSTALLPATH")
                set(_state "installpath")
            elseif (${arg} STREQUAL "DOCTITLE")
                set(_state "doctitle")
            elseif (${arg} STREQUAL "WINDOWTITLE")
                set(_state "windowtitle")
            elseif (${arg} STREQUAL "AUTHOR")
                set(_state "author")
            elseif (${arg} STREQUAL "USE")
                set(_state "use")
            elseif (${arg} STREQUAL "VERSION")
                set(_state "version")
            else ()
                list(APPEND _javadoc_sourcepath ${arg})
            endif ()
        elseif (${_state} STREQUAL "classpath")
            if (${arg} STREQUAL "PACKAGES")
                set(_state "packages")
            elseif (${arg} STREQUAL "FILES")
                set(_state "files")
            elseif (${arg} STREQUAL "SOURCEPATH")
                set(_state "sourcepath")
            elseif (${arg} STREQUAL "INSTALLPATH")
                set(_state "installpath")
            elseif (${arg} STREQUAL "DOCTITLE")
                set(_state "doctitle")
            elseif (${arg} STREQUAL "WINDOWTITLE")
                set(_state "windowtitle")
            elseif (${arg} STREQUAL "AUTHOR")
                set(_state "author")
            elseif (${arg} STREQUAL "USE")
                set(_state "use")
            elseif (${arg} STREQUAL "VERSION")
                set(_state "version")
            else ()
                list(APPEND _javadoc_classpath ${arg})
            endif ()
        elseif (${_state} STREQUAL "installpath")
            if (${arg} STREQUAL "PACKAGES")
                set(_state "packages")
            elseif (${arg} STREQUAL "FILES")
                set(_state "files")
            elseif (${arg} STREQUAL "SOURCEPATH")
                set(_state "sourcepath")
            elseif (${arg} STREQUAL "DOCTITLE")
                set(_state "doctitle")
            elseif (${arg} STREQUAL "WINDOWTITLE")
                set(_state "windowtitle")
            elseif (${arg} STREQUAL "AUTHOR")
                set(_state "author")
            elseif (${arg} STREQUAL "USE")
                set(_state "use")
            elseif (${arg} STREQUAL "VERSION")
                set(_state "version")
            else ()
                set(_javadoc_installpath ${arg})
            endif ()
        elseif (${_state} STREQUAL "doctitle")
            if (${arg} STREQUAL "PACKAGES")
                set(_state "packages")
            elseif (${arg} STREQUAL "FILES")
                set(_state "files")
            elseif (${arg} STREQUAL "SOURCEPATH")
                set(_state "sourcepath")
            elseif (${arg} STREQUAL "INSTALLPATH")
                set(_state "installpath")
            elseif (${arg} STREQUAL "CLASSPATH")
                set(_state "classpath")
            elseif (${arg} STREQUAL "WINDOWTITLE")
                set(_state "windowtitle")
            elseif (${arg} STREQUAL "AUTHOR")
                set(_state "author")
            elseif (${arg} STREQUAL "USE")
                set(_state "use")
            elseif (${arg} STREQUAL "VERSION")
                set(_state "version")
            else ()
                set(_javadoc_doctitle ${arg})
            endif ()
        elseif (${_state} STREQUAL "windowtitle")
            if (${arg} STREQUAL "PACKAGES")
                set(_state "packages")
            elseif (${arg} STREQUAL "FILES")
                set(_state "files")
            elseif (${arg} STREQUAL "SOURCEPATH")
                set(_state "sourcepath")
            elseif (${arg} STREQUAL "CLASSPATH")
                set(_state "classpath")
            elseif (${arg} STREQUAL "INSTALLPATH")
                set(_state "installpath")
            elseif (${arg} STREQUAL "DOCTITLE")
                set(_state "doctitle")
            elseif (${arg} STREQUAL "AUTHOR")
                set(_state "author")
            elseif (${arg} STREQUAL "USE")
                set(_state "use")
            elseif (${arg} STREQUAL "VERSION")
                set(_state "version")
            else ()
                set(_javadoc_windowtitle ${arg})
            endif ()
        elseif (${_state} STREQUAL "author")
            if (${arg} STREQUAL "PACKAGES")
                set(_state "packages")
            elseif (${arg} STREQUAL "FILES")
                set(_state "files")
            elseif (${arg} STREQUAL "SOURCEPATH")
                set(_state "sourcepath")
            elseif (${arg} STREQUAL "CLASSPATH")
                set(_state "classpath")
            elseif (${arg} STREQUAL "INSTALLPATH")
                set(_state "installpath")
            elseif (${arg} STREQUAL "DOCTITLE")
                set(_state "doctitle")
            elseif (${arg} STREQUAL "WINDOWTITLE")
                set(_state "windowtitle")
            elseif (${arg} STREQUAL "AUTHOR")
                set(_state "author")
            elseif (${arg} STREQUAL "USE")
                set(_state "use")
            elseif (${arg} STREQUAL "VERSION")
                set(_state "version")
            else ()
                set(_javadoc_author ${arg})
            endif ()
        elseif (${_state} STREQUAL "use")
            if (${arg} STREQUAL "PACKAGES")
                set(_state "packages")
            elseif (${arg} STREQUAL "FILES")
                set(_state "files")
            elseif (${arg} STREQUAL "SOURCEPATH")
                set(_state "sourcepath")
            elseif (${arg} STREQUAL "CLASSPATH")
                set(_state "classpath")
            elseif (${arg} STREQUAL "INSTALLPATH")
                set(_state "installpath")
            elseif (${arg} STREQUAL "DOCTITLE")
                set(_state "doctitle")
            elseif (${arg} STREQUAL "WINDOWTITLE")
                set(_state "windowtitle")
            elseif (${arg} STREQUAL "AUTHOR")
                set(_state "author")
            elseif (${arg} STREQUAL "USE")
                set(_state "use")
            elseif (${arg} STREQUAL "VERSION")
                set(_state "version")
            else ()
                set(_javadoc_use ${arg})
            endif ()
        elseif (${_state} STREQUAL "version")
            if (${arg} STREQUAL "PACKAGES")
                set(_state "packages")
            elseif (${arg} STREQUAL "FILES")
                set(_state "files")
            elseif (${arg} STREQUAL "SOURCEPATH")
                set(_state "sourcepath")
            elseif (${arg} STREQUAL "CLASSPATH")
                set(_state "classpath")
            elseif (${arg} STREQUAL "INSTALLPATH")
                set(_state "installpath")
            elseif (${arg} STREQUAL "DOCTITLE")
                set(_state "doctitle")
            elseif (${arg} STREQUAL "WINDOWTITLE")
                set(_state "windowtitle")
            elseif (${arg} STREQUAL "AUTHOR")
                set(_state "author")
            elseif (${arg} STREQUAL "USE")
                set(_state "use")
            elseif (${arg} STREQUAL "VERSION")
                set(_state "version")
            else ()
                set(_javadoc_version ${arg})
            endif ()
        endif (${_state} STREQUAL "package")
    endforeach (arg ${ARGN})

    set(_javadoc_builddir ${CMAKE_CURRENT_BINARY_DIR}/javadoc/${_target})
    set(_javadoc_options -d ${_javadoc_builddir})

    if (_javadoc_sourcepath)
        set(_start TRUE)
        foreach(_path ${_javadoc_sourcepath})
            if (_start)
                set(_sourcepath ${_path})
                set(_start FALSE)
            else (_start)
                set(_sourcepath ${_sourcepath}:${_path})
            endif (_start)
        endforeach(_path ${_javadoc_sourcepath})
        set(_javadoc_options ${_javadoc_options} -sourcepath ${_sourcepath})
    endif (_javadoc_sourcepath)

    if (_javadoc_classpath)
        set(_start TRUE)
        foreach(_path ${_javadoc_classpath})
            if (_start)
                set(_classpath ${_path})
                set(_start FALSE)
            else (_start)
                set(_classpath ${_classpath}:${_path})
            endif (_start)
        endforeach(_path ${_javadoc_classpath})
        set(_javadoc_options ${_javadoc_options} -classpath "${_classpath}")
    endif (_javadoc_classpath)

    if (_javadoc_doctitle)
        set(_javadoc_options ${_javadoc_options} -doctitle '${_javadoc_doctitle}')
    endif (_javadoc_doctitle)

    if (_javadoc_windowtitle)
        set(_javadoc_options ${_javadoc_options} -windowtitle '${_javadoc_windowtitle}')
    endif (_javadoc_windowtitle)

    if (_javadoc_author)
        set(_javadoc_options ${_javadoc_options} -author)
    endif (_javadoc_author)

    if (_javadoc_use)
        set(_javadoc_options ${_javadoc_options} -use)
    endif (_javadoc_use)

    if (_javadoc_version)
        set(_javadoc_options ${_javadoc_options} -version)
    endif (_javadoc_version)

    add_custom_target(${_target}_javadoc ALL
        COMMAND ${Java_JAVADOC_EXECUTABLE} ${_javadoc_options}
                            ${_javadoc_files}
                            ${_javadoc_packages}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    install(
        DIRECTORY ${_javadoc_builddir}
        DESTINATION ${_javadoc_installpath}
    )
endfunction(create_javadoc)
