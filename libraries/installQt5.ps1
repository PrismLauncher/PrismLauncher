$aqt_url = "https://github.com/miurahr/aqtinstall/releases/latest/download/aqt.exe"
$qt_path = ( Join-Path -Path "$PSScriptRoot" -ChildPath "Qt" )
New-Item -ItemType Directory -Path $qt_path -ErrorAction SilentlyContinue
$aqt_path = ( Join-Path -Path $qt_path -ChildPath "aqt.exe" )

if (!(Test-Path -Path $aqt_path)) {
  Invoke-WebRequest -Uri $aqt_url -OutFile $aqt_path
}

$qt_ver = "5.15.2"
$qt_arch = "win32_msvc2019"
$aqt_install_args = @("install-qt", "windows", "desktop", $qt_ver, $qt_arch, "-m", "all", "--outputdir", $qt_path )

$p = Start-Process -FilePath $aqt_path -ArgumentList $aqt_install_args -Wait


