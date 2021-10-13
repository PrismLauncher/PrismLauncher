add_library(secrets STATIC Secrets.cpp Secrets.h)
target_link_libraries(secrets Qt5::Core)
target_compile_definitions(secrets PUBLIC -DEMBED_SECRETS)
target_include_directories(secrets PUBLIC .)

set(Launcher_Name "Launcher" PARENT_SCOPE)
set(Launcher_Copyright "MultiMC Contributors" PARENT_SCOPE)

set(Launcher_Branding_ICNS "notsecrets/Launcher.icns" PARENT_SCOPE)
set(Launcher_Branding_WindowsRC "notsecrets/launcher.rc" PARENT_SCOPE)
