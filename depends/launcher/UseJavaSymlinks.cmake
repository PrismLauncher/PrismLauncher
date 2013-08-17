#
# Helper script for UseJava.cmake
#

#=============================================================================
# Copyright 2010-2011 Andreas schneider <asn@redhat.com>
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

if (UNIX AND _JAVA_TARGET_OUTPUT_LINK)
    if (_JAVA_TARGET_OUTPUT_NAME)
        find_program(LN_EXECUTABLE
            NAMES
                ln
        )

        execute_process(
            COMMAND ${LN_EXECUTABLE} -sf "${_JAVA_TARGET_OUTPUT_NAME}" "${_JAVA_TARGET_OUTPUT_LINK}"
            WORKING_DIRECTORY ${_JAVA_TARGET_DIR}
        )
    else (_JAVA_TARGET_OUTPUT_NAME)
        message(SEND_ERROR "FATAL: Can't find _JAVA_TARGET_OUTPUT_NAME")
    endif (_JAVA_TARGET_OUTPUT_NAME)
endif (UNIX AND _JAVA_TARGET_OUTPUT_LINK)
