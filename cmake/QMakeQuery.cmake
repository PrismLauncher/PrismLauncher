if(__QMAKEQUERY_CMAKE__)
    return()
endif()
set(__QMAKEQUERY_CMAKE__ TRUE)

get_target_property(QMAKE_EXECUTABLE Qt5::qmake LOCATION)

function(QUERY_QMAKE VAR RESULT)
    exec_program(${QMAKE_EXECUTABLE} ARGS "-query ${VAR}" RETURN_VALUE return_code OUTPUT_VARIABLE output )
    if(NOT return_code)
        file(TO_CMAKE_PATH "${output}" output)
        set(${RESULT} ${output} PARENT_SCOPE)
    endif(NOT return_code)
endfunction(QUERY_QMAKE)
