# Builds the React web UI (frontend/dist) and stages it next to the launcher binary.
#
# Inputs:
#   NPM_EXECUTABLE              npm used to install and build (found automatically)
#   Launcher_FRONTEND_DIST_DIR  optional prebuilt dist/ directory; when set, npm is not used at all
#                               (for offline/distro builds that build the frontend separately)
# Outputs:
#   target  Launcher_frontend   builds and copies the bundle to ${Launcher_FRONTEND_STAGING_DIR}
#   install the bundle to ${FRONTEND_DEST_DIR}

set(Launcher_FRONTEND_DIST_DIR "" CACHE PATH "Use a prebuilt frontend dist/ directory instead of running npm")

set(FRONTEND_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(FRONTEND_STAGING_DIR "${CMAKE_BINARY_DIR}/frontend")
set(Launcher_FRONTEND_STAGING_DIR "${FRONTEND_STAGING_DIR}" PARENT_SCOPE)
set(FRONTEND_STAMP "${CMAKE_CURRENT_BINARY_DIR}/frontend.stamp")

if(Launcher_FRONTEND_DIST_DIR)
    if(NOT EXISTS "${Launcher_FRONTEND_DIST_DIR}/index.html")
        message(FATAL_ERROR "Launcher_FRONTEND_DIST_DIR='${Launcher_FRONTEND_DIST_DIR}' does not contain index.html")
    endif()
    message(STATUS "Web UI: using prebuilt frontend from ${Launcher_FRONTEND_DIST_DIR}")
    file(GLOB_RECURSE FRONTEND_DIST_FILES CONFIGURE_DEPENDS "${Launcher_FRONTEND_DIST_DIR}/*")
    add_custom_command(
        OUTPUT "${FRONTEND_STAMP}"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${FRONTEND_STAGING_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${Launcher_FRONTEND_DIST_DIR}" "${FRONTEND_STAGING_DIR}"
        COMMAND ${CMAKE_COMMAND} -E touch "${FRONTEND_STAMP}"
        DEPENDS ${FRONTEND_DIST_FILES}
        COMMENT "Staging prebuilt web UI"
        VERBATIM
    )
else()
    find_program(NPM_EXECUTABLE NAMES npm npm.cmd)
    if(NOT NPM_EXECUTABLE)
        message(FATAL_ERROR
            "Web UI: npm was not found. Install Node.js (>= 20), pass -DLauncher_FRONTEND_DIST_DIR=<prebuilt dist>, "
            "or disable the web UI with -DLauncher_ENABLE_WEBUI=OFF.")
    endif()
    message(STATUS "Web UI: building frontend with ${NPM_EXECUTABLE}")

    # Reinstall node_modules only when the lock file changes.
    set(FRONTEND_DEPS_STAMP "${FRONTEND_SOURCE_DIR}/node_modules/.package-lock.json")
    add_custom_command(
        OUTPUT "${FRONTEND_DEPS_STAMP}"
        COMMAND "${NPM_EXECUTABLE}" ci --no-audit --no-fund
        WORKING_DIRECTORY "${FRONTEND_SOURCE_DIR}"
        DEPENDS "${FRONTEND_SOURCE_DIR}/package.json" "${FRONTEND_SOURCE_DIR}/package-lock.json"
        COMMENT "Installing web UI dependencies (npm ci)"
        VERBATIM
    )

    file(GLOB_RECURSE FRONTEND_SOURCES CONFIGURE_DEPENDS
        "${FRONTEND_SOURCE_DIR}/src/*"
        "${FRONTEND_SOURCE_DIR}/public/*"
    )
    list(APPEND FRONTEND_SOURCES
        "${FRONTEND_SOURCE_DIR}/index.html"
        "${FRONTEND_SOURCE_DIR}/vite.config.ts"
        "${FRONTEND_SOURCE_DIR}/tsconfig.json"
    )

    add_custom_command(
        OUTPUT "${FRONTEND_STAMP}"
        # Type-check + bundle into frontend/dist
        COMMAND "${NPM_EXECUTABLE}" run build
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${FRONTEND_STAGING_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${FRONTEND_SOURCE_DIR}/dist" "${FRONTEND_STAGING_DIR}"
        COMMAND ${CMAKE_COMMAND} -E touch "${FRONTEND_STAMP}"
        WORKING_DIRECTORY "${FRONTEND_SOURCE_DIR}"
        DEPENDS "${FRONTEND_DEPS_STAMP}" ${FRONTEND_SOURCES}
        COMMENT "Building web UI (npm run build)"
        VERBATIM
    )
endif()

add_custom_target(Launcher_frontend ALL DEPENDS "${FRONTEND_STAMP}")

install(DIRECTORY "${FRONTEND_STAGING_DIR}/" DESTINATION "${FRONTEND_DEST_DIR}" COMPONENT Runtime)
