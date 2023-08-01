$aqt_url = "https://github.com/miurahr/aqtinstall/releases/latest/download/aqt.exe"
$qt_path = ( Join-Path -Path "$PSScriptRoot/libraries" -ChildPath "Qt" )
New-Item -ItemType Directory -Path $qt_path -ErrorAction SilentlyContinue
$aqt_path = ( Join-Path -Path $qt_path -ChildPath "aqt.exe" )

if (!(Test-Path -Path $aqt_path)) {
  Invoke-WebRequest -Uri $aqt_url -OutFile $aqt_path
}

$qt_ver = "6.5.1"
$system_arch = (Get-CimInstance -Class Win32_ComputerSystem).SystemType
$qt_arch = Switch ($system_arch) {
  {$_ -match "x64" } { "win64_msvc2019_64" }
  {$_ -match "ARM" } { "win64_msvc2019_arm64" }
}

$aqt_install_args = @("install-qt", "windows", "desktop", $qt_ver, $qt_arch, "-m", "all", "--outputdir", $qt_path )

$p = Start-Process -FilePath $aqt_path -ArgumentList $aqt_install_args -Wait


