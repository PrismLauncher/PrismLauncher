!define MULTIUSER_EXECUTIONLEVEL Highest
!define MULTIUSER_MUI
!define MULTIUSER_INSTALLMODE_COMMANDLINE

!define MULTIUSER_INSTALLMODE_INSTDIR PolyMC
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_KEY Software\PolyMC
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_VALUENAME InstallDir

!include "FileFunc.nsh"
!include "MUI2.nsh"
!include "MultiUser.nsh"

Name "PolyMC"
RequestExecutionLevel highest

;--------------------------------

; Pages

!insertmacro MUI_PAGE_WELCOME
!insertmacro MULTIUSER_PAGE_INSTALLMODE
!define MUI_COMPONENTSPAGE_NODESC
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$InstDir\polymc.exe"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

;--------------------------------

; The stuff to install
Section "PolyMC"

  SectionIn RO

  nsExec::Exec /TIMEOUT=2000 'TaskKill /IM polymc.exe /F'

  SetOutPath $INSTDIR

  File "polymc.exe"
  File "qt.conf"
  File *.dll
  File /r "iconengines"
  File /r "imageformats"
  File /r "jars"
  File /r "platforms"
  File /r "styles"

  ; Write the installation path into the registry
  WriteRegStr SHCTX SOFTWARE\PolyMC "InstallDir" "$INSTDIR"

  ; Write the uninstall keys for Windows
  !define UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\PolyMC"
  WriteRegStr SHCTX "${UNINST_KEY}" "DisplayName" "PolyMC"
  WriteRegStr SHCTX "${UNINST_KEY}" "DisplayIcon" "$INSTDIR\polymc.exe"
  WriteRegStr SHCTX "${UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe" /$MultiUser.InstallMode'
  WriteRegStr SHCTX "${UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /$MultiUser.InstallMode /S'
  WriteRegStr SHCTX "${UNINST_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr SHCTX "${UNINST_KEY}" "Publisher" "PolyMC Contributors"
  WriteRegStr SHCTX "${UNINST_KEY}" "ProductVersion" "${VERSION}"
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD SHCTX "${UNINST_KEY}" "EstimatedSize" "$0"
  WriteRegDWORD SHCTX "${UNINST_KEY}" "NoModify" 1
  WriteRegDWORD SHCTX "${UNINST_KEY}" "NoRepair" 1
  WriteUninstaller "$INSTDIR\uninstall.exe"

SectionEnd

Section "Start Menu Shortcuts"

  CreateShortcut "$SMPROGRAMS\PolyMC.lnk" "$INSTDIR\polymc.exe" "" "$INSTDIR\polymc.exe" 0

SectionEnd

Section /o "Portable"

  SetOutPath $INSTDIR
  File "portable.txt"

SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"

  nsExec::Exec /TIMEOUT=2000 'TaskKill /IM polymc.exe /F'

  DeleteRegKey SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\PolyMC"
  DeleteRegKey SHCTX SOFTWARE\PolyMC

  Delete $INSTDIR\polymc.exe
  Delete $INSTDIR\uninstall.exe
  Delete $INSTDIR\portable.txt

  Delete $INSTDIR\libbrotlicommon.dll
  Delete $INSTDIR\libbrotlidec.dll
  Delete $INSTDIR\libbz2-1.dll
  Delete $INSTDIR\libcrypto-1_1-x64.dll
  Delete $INSTDIR\libcrypto-1_1.dll
  Delete $INSTDIR\libdouble-conversion.dll
  Delete $INSTDIR\libfreetype-6.dll
  Delete $INSTDIR\libgcc_s_seh-1.dll
  Delete $INSTDIR\libgcc_s_dw2-1.dll
  Delete $INSTDIR\libglib-2.0-0.dll
  Delete $INSTDIR\libgraphite2.dll
  Delete $INSTDIR\libharfbuzz-0.dll
  Delete $INSTDIR\libiconv-2.dll
  Delete $INSTDIR\libicudt69.dll
  Delete $INSTDIR\libicuin69.dll
  Delete $INSTDIR\libicuuc69.dll
  Delete $INSTDIR\libintl-8.dll
  Delete $INSTDIR\libjasper-4.dll
  Delete $INSTDIR\libjpeg-8.dll
  Delete $INSTDIR\libmd4c.dll
  Delete $INSTDIR\libpcre-1.dll
  Delete $INSTDIR\libpcre2-16-0.dll
  Delete $INSTDIR\libpng16-16.dll
  Delete $INSTDIR\libssl-1_1-x64.dll
  Delete $INSTDIR\libssl-1_1.dll
  Delete $INSTDIR\libssp-0.dll
  Delete $INSTDIR\libstdc++-6.dll
  Delete $INSTDIR\libwebp-7.dll
  Delete $INSTDIR\libwebpdemux-2.dll
  Delete $INSTDIR\libwebpmux-3.dll
  Delete $INSTDIR\libwinpthread-1.dll
  Delete $INSTDIR\libzstd.dll
  Delete $INSTDIR\Qt5Core.dll
  Delete $INSTDIR\Qt5Gui.dll
  Delete $INSTDIR\Qt5Network.dll
  Delete $INSTDIR\Qt5Qml.dll
  Delete $INSTDIR\Qt5QmlModels.dll
  Delete $INSTDIR\Qt5Quick.dll
  Delete $INSTDIR\Qt5Svg.dll
  Delete $INSTDIR\Qt5WebSockets.dll
  Delete $INSTDIR\Qt5Widgets.dll
  Delete $INSTDIR\Qt5Xml.dll
  Delete $INSTDIR\zlib1.dll

  Delete $INSTDIR\qt.conf

  RMDir /r $INSTDIR\iconengines
  RMDir /r $INSTDIR\imageformats
  RMDir /r $INSTDIR\jars
  RMDir /r $INSTDIR\platforms
  RMDir /r $INSTDIR\styles

  Delete "$SMPROGRAMS\PolyMC.lnk"

  RMDir "$INSTDIR"

SectionEnd

; Multi-user

Function .onInit
  !insertmacro MULTIUSER_INIT
FunctionEnd

Function un.onInit
  !insertmacro MULTIUSER_UNINIT
FunctionEnd
