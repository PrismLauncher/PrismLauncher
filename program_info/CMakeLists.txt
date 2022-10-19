if(UNIX)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_search_module(SCDOC scdoc)
        if(SCDOC_FOUND)
            pkg_get_variable(SCDOC_SCDOC scdoc scdoc)
        endif()
    endif()
endif()

set(Launcher_CommonName "PrismLauncher")
set(Launcher_DisplayName "Prism Launcher")

set(Launcher_Name "${Launcher_CommonName}" PARENT_SCOPE)
set(Launcher_DisplayName "${Launcher_DisplayName}" PARENT_SCOPE)
set(Launcher_LoggingCategory "prismlauncher" PARENT_SCOPE)

set(Launcher_Copyright "Prism Launcher Contributors\\n© 2012-2021 MultiMC Contributors")
set(Launcher_Copyright "${Launcher_Copyright}" PARENT_SCOPE)
set(Launcher_Domain "prismlauncher.org" PARENT_SCOPE)
set(Launcher_UserAgent "${Launcher_CommonName}/${Launcher_VERSION_NAME}" PARENT_SCOPE)
set(Launcher_ConfigFile "prismlauncher.cfg" PARENT_SCOPE)
set(Launcher_Git "https://github.com/PrismLauncher/PrismLauncher" PARENT_SCOPE)
set(Launcher_DesktopFileName "org.prismlauncher.PrismLauncher.desktop" PARENT_SCOPE)

set(Launcher_Desktop "program_info/org.prismlauncher.PrismLauncher.desktop" PARENT_SCOPE)
set(Launcher_MetaInfo "program_info/org.prismlauncher.PrismLauncher.metainfo.xml" PARENT_SCOPE)
set(Launcher_SVG "program_info/org.prismlauncher.PrismLauncher.svg" PARENT_SCOPE)
set(Launcher_Branding_ICNS "program_info/prismlauncher.icns" PARENT_SCOPE)
set(Launcher_Branding_ICO "program_info/prismlauncher.ico")
set(Launcher_Branding_ICO "${Launcher_Branding_ICO}" PARENT_SCOPE)
set(Launcher_Branding_WindowsRC "program_info/prismlauncher.rc" PARENT_SCOPE)
set(Launcher_Branding_LogoQRC "program_info/prismlauncher.qrc" PARENT_SCOPE)

set(Launcher_Portable_File "program_info/portable.txt" PARENT_SCOPE)

configure_file(org.prismlauncher.PrismLauncher.desktop.in org.prismlauncher.PrismLauncher.desktop)
configure_file(org.prismlauncher.PrismLauncher.metainfo.xml.in org.prismlauncher.PrismLauncher.metainfo.xml)
configure_file(prismlauncher.rc.in prismlauncher.rc @ONLY)
configure_file(prismlauncher.manifest.in prismlauncher.manifest @ONLY)
configure_file(prismlauncher.ico prismlauncher.ico COPYONLY)
configure_file(win_install.nsi.in win_install.nsi @ONLY)

if(SCDOC_FOUND)
    set(in_scd "${CMAKE_CURRENT_SOURCE_DIR}/prismlauncher.6.scd")
    set(out_man "${CMAKE_CURRENT_BINARY_DIR}/prismlauncher.6")
    add_custom_command(
        DEPENDS "${in_scd}"
        OUTPUT "${out_man}"
        COMMAND ${SCDOC_SCDOC} < "${in_scd}" > "${out_man}"
    )
    add_custom_target(man ALL DEPENDS ${out_man})
    set(Launcher_ManPage "program_info/prismlauncher.6" PARENT_SCOPE)
endif()
