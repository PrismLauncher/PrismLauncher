#=============================================================================
# Copyright 2005-2011 Kitware, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of Kitware, Inc. nor the names of its
#   contributors may be used to endorse or promote products derived
#   from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

# From Qt5CoreMacros.cmake

function(qt_generate_moc)
    if(QT_VERSION_MAJOR EQUAL 5)
        qt5_generate_moc(${ARGV})
    elseif(QT_VERSION_MAJOR EQUAL 6)
        qt6_generate_moc(${ARGV})
    endif()
endfunction()

function(qt_wrap_cpp outfiles)
    if(QT_VERSION_MAJOR EQUAL 5)
        qt5_wrap_cpp("${outfiles}" ${ARGN})
    elseif(QT_VERSION_MAJOR EQUAL 6)
        qt6_wrap_cpp("${outfiles}" ${ARGN})
    endif()
    set("${outfiles}" "${${outfiles}}" PARENT_SCOPE)
endfunction()

function(qt_add_binary_resources)
    if(QT_VERSION_MAJOR EQUAL 5)
        qt5_add_binary_resources(${ARGV})
    elseif(QT_VERSION_MAJOR EQUAL 6)
        qt6_add_binary_resources(${ARGV})
    endif()
endfunction()

function(qt_add_resources outfiles)
    if(QT_VERSION_MAJOR EQUAL 5)
        qt5_add_resources("${outfiles}" ${ARGN})
    elseif(QT_VERSION_MAJOR EQUAL 6)
        qt6_add_resources("${outfiles}" ${ARGN})
    endif()
    set("${outfiles}" "${${outfiles}}" PARENT_SCOPE)
endfunction()

function(qt_add_big_resources outfiles)
    if(QT_VERSION_MAJOR EQUAL 5)
        qt5_add_big_resources(${outfiles} ${ARGN})
    elseif(QT_VERSION_MAJOR EQUAL 6)
        qt6_add_big_resources(${outfiles} ${ARGN})
    endif()
    set("${outfiles}" "${${outfiles}}" PARENT_SCOPE)
endfunction()

function(qt_import_plugins)
    if(QT_VERSION_MAJOR EQUAL 5)
        qt5_import_plugins(${ARGV})
    elseif(QT_VERSION_MAJOR EQUAL 6)
        qt6_import_plugins(${ARGV})
    endif()
endfunction()


# From Qt5WidgetsMacros.cmake

function(qt_wrap_ui outfiles)
    if(QT_VERSION_MAJOR EQUAL 5)
        qt5_wrap_ui("${outfiles}" ${ARGN})
    elseif(QT_VERSION_MAJOR EQUAL 6)
        qt6_wrap_ui("${outfiles}" ${ARGN})
    endif()
    set("${outfiles}" "${${outfiles}}" PARENT_SCOPE)
endfunction()

