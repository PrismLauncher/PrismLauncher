set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

# NOTE: This is the only change we're making to the default target
#
# Avoid using the debug VCRUNTIME. Otherwise, this would introduce the Windows SDK
# as a requirement for testers :(
# We also don't really need the debug versions of our libraries anyways
set(VCPKG_BUILD_TYPE release)
