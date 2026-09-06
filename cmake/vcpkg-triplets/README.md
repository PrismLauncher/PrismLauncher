# vcpkg-triplets

These are here to ensure that the `-static-release` equivalents of default triplets are used, and avoid the mess of manually specifying them in CMakePresets.json (where there's no macro for getting the current architecture) or in some random .env file
