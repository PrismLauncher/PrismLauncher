# 
# Function to set compiler warnings with reasonable defaults at the project level.
# Taken from https://github.com/aminya/project_options/blob/main/src/CompilerWarnings.cmake
# under the folowing license:
# 
# MIT License
#
# Copyright (c) 2022-2100 Amin Yahyaabadi
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

include_guard()

function(_set_project_warnings_add_target_link_option TARGET OPTIONS)
  target_link_options(${_project_name} INTERFACE ${OPTIONS})
endfunction()

# Set the compiler warnings
#
# https://clang.llvm.org/docs/DiagnosticsReference.html
# https://github.com/lefticus/cppbestpractices/blob/master/02-Use_the_Tools_Available.md
function(
  set_project_warnings
  _project_name
  MSVC_WARNINGS
  CLANG_WARNINGS
  GCC_WARNINGS
)
  if("${MSVC_WARNINGS}" STREQUAL "")
    set(MSVC_WARNINGS
        /W4 # Baseline reasonable warnings
        /w14242 # 'identifier': conversion from 'type1' to 'type1', possible loss of data
        /w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
        /w14263 # 'function': member function does not override any base class virtual member function
        /w14265 # 'classname': class has virtual functions, but destructor is not virtual instances of this class may not
                # be destructed correctly
        /w14287 # 'operator': unsigned/negative constant mismatch
        /we4289 # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside
                # the for-loop scope
        /w14296 # 'operator': expression is always 'boolean_value'
        /w14311 # 'variable': pointer truncation from 'type1' to 'type2'
        /w14545 # expression before comma evaluates to a function which is missing an argument list
        /w14546 # function call before comma missing argument list
        /w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
        /w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
        /w14555 # expression has no effect; expected expression with side- effect
        /w14619 # pragma warning: there is no warning number 'number'
        /w14640 # Enable warning on thread un-safe static member initialization
        /w14826 # Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behavior.
        /w14905 # wide string literal cast to 'LPSTR'
        /w14906 # string literal cast to 'LPWSTR'
        /w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
        /permissive- # standards conformance mode for MSVC compiler.
    )
  endif()

  if("${CLANG_WARNINGS}" STREQUAL "")
    set(CLANG_WARNINGS
        -Wall
        -Wextra # reasonable and standard
        -Wshadow # warn the user if a variable declaration shadows one from a parent context
        -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor. This helps
        # catch hard to track down memory errors
        -Wold-style-cast # warn for c-style casts
        -Wcast-align # warn for potential performance problem casts
        -Wunused # warn on anything being unused
        -Woverloaded-virtual # warn if you overload (not override) a virtual function
        -Wpedantic # warn if non-standard C++ is used
        -Wconversion # warn on type conversions that may lose data
        -Wsign-conversion # warn on sign conversions
        -Wnull-dereference # warn if a null dereference is detected
        -Wdouble-promotion # warn if float is implicit promoted to double
        -Wformat=2 # warn on security issues around functions that format output (ie printf)
        -Wimplicit-fallthrough # warn on statements that fallthrough without an explicit annotation
    )
  endif()

  if("${GCC_WARNINGS}" STREQUAL "")
    set(GCC_WARNINGS
        ${CLANG_WARNINGS}
        -Wmisleading-indentation # warn if indentation implies blocks where blocks do not exist
        -Wduplicated-cond # warn if if / else chain has duplicated conditions
        -Wduplicated-branches # warn if if / else branches have duplicated code
        -Wlogical-op # warn about logical operations being used where bitwise were probably wanted
        -Wuseless-cast # warn if you perform a cast to the same type
    )
  endif()

  if(MSVC)
    set(PROJECT_WARNINGS_CXX ${MSVC_WARNINGS})
  elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    set(PROJECT_WARNINGS_CXX ${CLANG_WARNINGS})
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(PROJECT_WARNINGS_CXX ${GCC_WARNINGS})
  else()
    message(AUTHOR_WARNING "No compiler warnings set for CXX compiler: '${CMAKE_CXX_COMPILER_ID}'")
    # TODO support Intel compiler
  endif()

  # Add C warnings
  set(PROJECT_WARNINGS_C "${PROJECT_WARNINGS_CXX}")
  list(
    REMOVE_ITEM
    PROJECT_WARNINGS_C
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Woverloaded-virtual
    -Wuseless-cast
    -Wextra-semi
  )

  target_compile_options(
    ${_project_name}
    INTERFACE # C++ warnings
              $<$<COMPILE_LANGUAGE:CXX>:${PROJECT_WARNINGS_CXX}>
              # C warnings
              $<$<COMPILE_LANGUAGE:C>:${PROJECT_WARNINGS_C}>
   )

  # If we are using the compiler as a linker driver pass the warnings to it
  # (most useful when using LTO or warnings as errors)
  if(CMAKE_CXX_LINK_EXECUTABLE MATCHES "^<CMAKE_CXX_COMPILER>")
    _set_project_warnings_add_target_link_option(
      ${_project_name} "$<$<COMPILE_LANGUAGE:CXX>:${PROJECT_WARNINGS_CXX}>"
    )
  endif()

  if(CMAKE_C_LINK_EXECUTABLE MATCHES "^<CMAKE_C_COMPILER>")
    _set_project_warnings_add_target_link_option(
      ${_project_name} "$<$<COMPILE_LANGUAGE:C>:${PROJECT_WARNINGS_C}>"
    )
  endif()

 endfunction()
