set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_BUILD_TYPE release)

# HACK(@getchoo): We run into ABI compat issues when our dependencies aren't built with ASAN, but the main binary is
# So enable ASAN here too we know the launcher is using it
set(VCPKG_ENV_PASSTHROUGH DEBUG_ADDRESS_SANITIZER)
if(DEFINED ENV{DEBUG_ADDRESS_SANITIZER})
    if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        set(ASAN_FLAGS "/fsanitize=address /Oy-")
    else()
        set(ASAN_FLAGS "-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=null")
    endif()

    set(VCPKG_C_FLAGS ${ASAN_FLAGS})
    set(VCPKG_CXX_FLAGS ${ASAN_FLAGS})
endif()
