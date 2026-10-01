param(
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo')]
    [string]$Configuration = 'Release',
    [string]$QtRoot = '',
    [string]$JavaHome = '',
    [string]$InstallDir = '',
    [int]$Parallel = 8,
    [switch]$SkipSymlinkTests
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
if (!$InstallDir) { $InstallDir = Join-Path $repo 'install' }
$InstallDir = [System.IO.Path]::GetFullPath($InstallDir)
if (!$QtRoot) { $QtRoot = Join-Path $repo 'build-tools\Qt\6.11.2\msvc2022_64' }
if (!$JavaHome) {
    $jdk = Get-ChildItem -LiteralPath (Join-Path $repo 'build-tools\java17') -Directory | Select-Object -First 1
    if (!$jdk) { throw 'JDK 17 was not found. Pass -JavaHome with its directory.' }
    $JavaHome = $jdk.FullName
}
if (!(Test-Path -LiteralPath (Join-Path $QtRoot 'bin\qmake.exe'))) {
    throw 'Qt was not found. Pass -QtRoot with the MSVC Qt directory.'
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installations = & $vswhere -all -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json | ConvertFrom-Json
$vs = $installations | Where-Object {
    Test-Path -LiteralPath (Join-Path $_.installationPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
} | Select-Object -First 1
if (!$vs) { throw 'Visual Studio with the C++ and CMake components was not found.' }
$vsRoot = $vs.installationPath
& (Join-Path $vsRoot 'Common7\Tools\Launch-VsDevShell.ps1') -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
$cmakeDir = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake'
$cmake = Join-Path $cmakeDir 'CMake\bin\cmake.exe'
$ctest = Join-Path $cmakeDir 'CMake\bin\ctest.exe'
$ninja = Join-Path $cmakeDir 'Ninja\ninja.exe'
$env:VCPKG_ROOT = Join-Path $repo 'cmake\vcpkg'
$env:JAVA_HOME = $JavaHome
$env:PATH = "$QtRoot\bin;$JavaHome\bin;$env:PATH"

Push-Location -LiteralPath $repo
try {
    & $cmake --preset windows_msvc "-DCMAKE_MAKE_PROGRAM=$ninja" "-DCMAKE_PREFIX_PATH=$QtRoot" "-DCMAKE_INSTALL_PREFIX=$InstallDir" '-DLauncher_MSA_CLIENT_ID=' '-ULauncher_CURSEFORGE_API_KEY' '-DLauncher_IMGUR_CLIENT_ID=' '-DLauncher_BUILD_PLATFORM=RePrism-local' '-DENABLE_LTO=OFF'
    if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed ($LASTEXITCODE)." }
    & $cmake --build --preset windows_msvc --config $Configuration --parallel $Parallel
    if ($LASTEXITCODE -ne 0) { throw "Build failed ($LASTEXITCODE)." }
    if ($SkipSymlinkTests) {
        & $ctest --preset windows_msvc --build-config $Configuration --exclude-regex '^FileSystem$' --timeout 120
    } else {
        & $ctest --preset windows_msvc --build-config $Configuration --timeout 120
    }
    if ($LASTEXITCODE -ne 0) { throw "Tests failed ($LASTEXITCODE)." }
    if ($SkipSymlinkTests) {
        Write-Output 'Running FileSystem cases that do not require Windows symlink privileges.'
        & (Join-Path $repo "build\$Configuration\FileSystem.exe") test_pathCombine test_PathCombine1 test_PathCombine2 test_copy test_copy_with_blacklist test_copy_with_whitelist test_copy_with_dot_hidden test_copy_single_file test_getDesktop test_hard_link test_path_depth test_path_trunc
        if ($LASTEXITCODE -ne 0) { throw "FileSystem tests failed ($LASTEXITCODE)." }
    }
    & $cmake --install build --config $Configuration
    if ($LASTEXITCODE -ne 0) { throw "Installation failed ($LASTEXITCODE)." }
    & $cmake --install build --config $Configuration --component portable
    if ($LASTEXITCODE -ne 0) { throw "Portable installation failed ($LASTEXITCODE)." }
    Write-Output "Ready: $InstallDir\prismlauncher.exe"
} finally {
    Pop-Location
}
