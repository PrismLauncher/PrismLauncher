project(xpcservices C OBJC)

################################ FILES ################################

set(SANDBOXSERVICE_FILES
        SandboxService/main.m
        SandboxService/SandboxService.m
)

add_executable(SandboxService MACOSX_BUNDLE
        ${SANDBOXSERVICE_FILES})
target_link_libraries(SandboxService
        "-framework Foundation"
        "-framework UniformTypeIdentifiers"
        "-framework AppKit")
set_target_properties(SandboxService PROPERTIES
        BUNDLE True
        BUNDLE_EXTENSION "xpc"
        MACOSX_BUNDLE True
        MACOSX_BUNDLE_GUI_IDENTIFIER "${MACOSX_BUNDLE_GUI_IDENTIFIER}.PrismSandboxService"
        MACOSX_BUNDLE_BUNDLE_NAME "PrismSandboxService"
        MACOSX_BUNDLE_BUNDLE_VERSION "${MACOSX_BUNDLE_BUNDLE_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${MACOSX_BUNDLE_SHORT_VERSION_STRING}"
        MACOSX_BUNDLE_INFO_PLIST "MacOSXPCBundleInfo.plist.in"
        OUTPUT_NAME "PrismSandboxService"
)
target_compile_options(SandboxService PRIVATE
        -fobjc-arc
)
