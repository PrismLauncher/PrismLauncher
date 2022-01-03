set(Launcher_CommonName "PolyMC")

set(Launcher_Copyright "PolyMC Contributors" PARENT_SCOPE)
set(Launcher_Domain "github.com/PolyMC" PARENT_SCOPE)
set(Launcher_Name "${Launcher_CommonName}" PARENT_SCOPE)
set(Launcher_DisplayName "${Launcher_CommonName} 5" PARENT_SCOPE)
set(Launcher_UserAgent "${Launcher_CommonName}/5.0" PARENT_SCOPE)
set(Launcher_ConfigFile "polymc.cfg" PARENT_SCOPE)
set(Launcher_Git "https://github.com/PolyMC/PolyMC" PARENT_SCOPE)

set(Launcher_Desktop "program_info/org.polymc.PolyMC.desktop" PARENT_SCOPE)
set(Launcher_MetaInfo "program_info/org.polymc.PolyMC.metainfo.xml" PARENT_SCOPE)
set(Launcher_Branding_ICNS "program_info/polymc.icns" PARENT_SCOPE)
set(Launcher_Branding_WindowsRC "program_info/polymc.rc" PARENT_SCOPE)
set(Launcher_Branding_LogoQRC "program_info/polymc.qrc" PARENT_SCOPE)

configure_file(org.polymc.PolyMC.desktop.in org.polymc.PolyMC.desktop)
