if(UNIX)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_search_module(SCDOC scdoc)
        if(SCDOC_FOUND)
            pkg_get_variable(SCDOC_SCDOC scdoc scdoc)
        endif()
    endif()
endif()

set(Launcher_CommonName "PolyMC")

set(Launcher_Copyright "PolyMC Contributors\\n© 2012-2021 MultiMC Contributors")
set(Launcher_Copyright "${Launcher_Copyright}" PARENT_SCOPE)
set(Launcher_Domain "polymc.org" PARENT_SCOPE)
set(Launcher_Name "${Launcher_CommonName}" PARENT_SCOPE)
set(Launcher_DisplayName "${Launcher_CommonName}" PARENT_SCOPE)
set(Launcher_UserAgent "${Launcher_CommonName}/${Launcher_RELEASE_VERSION_NAME}" PARENT_SCOPE)
set(Launcher_ConfigFile "polymc.cfg" PARENT_SCOPE)
set(Launcher_Git "https://github.com/PolyMC/PolyMC" PARENT_SCOPE)
set(Launcher_DesktopFileName "org.polymc.PolyMC.desktop" PARENT_SCOPE)

set(Launcher_Desktop "program_info/org.polymc.PolyMC.desktop" PARENT_SCOPE)
set(Launcher_MetaInfo "program_info/org.polymc.PolyMC.metainfo.xml" PARENT_SCOPE)
set(Launcher_SVG "program_info/org.polymc.PolyMC.svg" PARENT_SCOPE)
set(Launcher_Branding_ICNS "program_info/polymc.icns" PARENT_SCOPE)
set(Launcher_Branding_ICO "program_info/polymc.ico")
set(Launcher_Branding_ICO "${Launcher_Branding_ICO}" PARENT_SCOPE)
set(Launcher_Branding_WindowsRC "program_info/polymc.rc" PARENT_SCOPE)
set(Launcher_Branding_LogoQRC "program_info/polymc.qrc" PARENT_SCOPE)

set(Launcher_Portable_File "program_info/portable.txt" PARENT_SCOPE)

configure_file(org.polymc.PolyMC.desktop.in org.polymc.PolyMC.desktop)
configure_file(org.polymc.PolyMC.metainfo.xml.in org.polymc.PolyMC.metainfo.xml)
configure_file(polymc.rc.in polymc.rc @ONLY)
configure_file(polymc.manifest.in polymc.manifest @ONLY)
configure_file(polymc.ico polymc.ico COPYONLY)
configure_file(win_install.nsi.in win_install.nsi @ONLY)

if(SCDOC_FOUND)
    set(in_scd "${CMAKE_CURRENT_SOURCE_DIR}/polymc.6.scd")
    set(out_man "${CMAKE_CURRENT_BINARY_DIR}/polymc.6")
    add_custom_command(
        DEPENDS "${in_scd}"
        OUTPUT "${out_man}"
        COMMAND ${SCDOC_SCDOC} < "${in_scd}" > "${out_man}"
    )
    add_custom_target(man ALL DEPENDS ${out_man})
    set(Launcher_ManPage "program_info/polymc.6" PARENT_SCOPE)
endif()
