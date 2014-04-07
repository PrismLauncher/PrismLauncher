if(__GITFUNCTIONS_CMAKE__)
    return()
endif()
set(__GITFUNCTIONS_CMAKE__ TRUE)

find_package(Git QUIET)

include(CMakeParseArguments)

if(GIT_FOUND)
    function(git_run)
	set(oneValueArgs OUTPUT_VAR DEFAULT)
	set(multiValueArgs COMMAND)
	cmake_parse_arguments(GIT_RUN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	execute_process(COMMAND ${GIT_EXECUTABLE} ${GIT_RUN_COMMAND}
	 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	 RESULT_VARIABLE GIT_RESULTVAR
	 OUTPUT_VARIABLE GIT_OUTVAR
	 OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	if(GIT_RESULTVAR EQUAL 0)
	    set(${GIT_RUN_OUTPUT_VAR} "${GIT_OUTVAR}" PARENT_SCOPE)
	else()
	    set(${GIT_RUN_OUTPUT_VAR} ${GIT_RUN_DEFAULT})
	    message(STATUS "Failed to run Git: ${GIT_OUTVAR}")
	endif()
    endfunction()
else()
    function(git_run)
	set(oneValueArgs OUTPUT_VAR DEFAULT)
	set(multiValueArgs COMMAND)
	cmake_parse_arguments(GIT_RUN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	set(${GIT_RUN_OUTPUT_VAR} ${GIT_RUN_DEFAULT})
    endfunction(git_run)
endif()
