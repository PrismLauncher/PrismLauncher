if(__COVERITY_CMAKE__)
    return()
endif()
set(__COVERITY_CMAKE__ TRUE)

include(GitFunctions)

git_run(COMMAND config --get user.email DEFAULT "" OUTPUT_VAR GIT_EMAIL)
git_run(COMMAND describe DEFAULT "" OUTPUT_VAR GIT_VERSION)

set(MultiMC_COVERITY_TOKEN "" CACHE STRING "Coverity access token")
set(MultiMC_COVERITY_EMAIL "${GIT_EMAIL}" CACHE STRING "Coverity email")

set(MultiMC_COVERITY_TOOLS_DIR "${CMAKE_BINARY_DIR}/coverity_tools" CACHE PATH "Path to the coverity tools")

find_program(CURL_EXECUTABLE NAMES curl PATHS /usr/bin)

if(NOT CURL_EXECUTABLE STREQUAL "" AND NOT MultiMC_COVERITY_TOKEN STREQUAL "" AND NOT MultiMC_COVERITY_EMAIL STREQUAL "")
    add_custom_target(coverity_configure
     COMMAND ${MultiMC_COVERITY_TOOLS_DIR}/bin/cov-configure --comptype gcc --compiler ${CMAKE_C_COMPILER}
    )
    add_custom_target(coverity_create_tarball
     COMMAND ${CMAKE_COMMAND} -E echo "Cleaning..." && ${CMAKE_MAKE_PROGRAM} clean
     COMMAND ${CMAKE_COMMAND} -E echo "Building..." && ${MultiMC_COVERITY_TOOLS_DIR}/bin/cov-build --dir cov-int ${CMAKE_MAKE_PROGRAM} -j3
     COMMAND ${CMAKE_COMMAND} -E echo "Creating tarball..." && ${CMAKE_COMMAND} -E tar cfz multimc_coverity.tgz cov-int/
     COMMENT "Creating coverity build..."
     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_custom_target(coverity_upload
     COMMAND ${CURL_EXECUTABLE} --form project=02JanDal/MultiMC5 --form token=${MultiMC_COVERITY_TOKEN} --form email=${MultiMC_COVERITY_EMAIL} --form file=@multimc_coverity.tgz --form version=${MultiMC_GIT_COMMIT} --form description=${GIT_VERSION} http://scan5.coverity.com/cgi-bin/upload.py
     DEPENDS coverity_create_tarball
     COMMENT "Uploading to coverity..."
     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
