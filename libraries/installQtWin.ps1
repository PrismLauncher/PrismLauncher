[cmdletbinding()]
param (
  [Parameter(Mandatory)][ValidateSet('ListQt', 'ListArch', 'ListModules', 'InstallQT')]
  [string]$Action,
  [Parameter(Mandatory=$false)]
  [string]$QtArch,
  [Parameter(Mandatory=$false)]
  [string]$QtVersion,
  [Parameter(Mandatory=$false)]
  [string[]]$AdtionalModules,
  [Parameter(Mandatory=$false)]
  [switch]$IncludeDefultDebugInfoModules = $false
)

$qt_v6_recomended = "6.7.0"
$qt_v5_recomended = "5.15.2"

$aqt_url = "https://github.com/miurahr/aqtinstall/releases/latest/download/aqt.exe"
$qt_path = ( Join-Path -Path "$PSScriptRoot" -ChildPath "Qt" )
New-Item -ItemType Directory -Path $qt_path -ErrorAction SilentlyContinue
$aqt_path = ( Join-Path -Path $qt_path -ChildPath "aqt.exe" )

if (!(Test-Path -Path $aqt_path))
{
  Invoke-WebRequest -Uri $aqt_url -OutFile $aqt_path
}

$qt_ver = Switch ($QtVersion)
{

  {$_ -eq "6"}
  {
    $qt_v6_recomended; Break
  }
  {$_ -eq "5"}
  {
    $qt_v5_recomended; Break
  }
  {$_ -match "\d\.\d+\.\d+"  }
  {
    $QtVersion; Break
  }
  {$_ -match "\d|\d\.\d+"}
  {
    Write-Host "Finding QtVersion ..."
    $aqt_list_args = @("list-qt", "windows", "desktop" )

    $pinfo = New-Object System.Diagnostics.ProcessStartInfo
    $pinfo.FileName = $aqt_path
    $pinfo.RedirectStandardOutput = $true
    $pinfo.RedirectStandardError = $true
    $pinfo.UseShellExecute = $false
    $pinfo.Arguments = $aqt_list_args
    $p = New-Object System.Diagnostics.Process
    $p.StartInfo = $pinfo
    $p.Start() | Out-Null
    $p.WaitForExit()
    $stdout = $p.StandardOutput.ReadToEnd()
    $versions = -split  $stdout | Where-Object {$_.StartsWith($QtVersion)}
    $latest = $versions | Measure-Object -Maximum | Select-Object -First 1 -ExpandProperty Maximum
    Write-Host "Using ${latest}"
    $latest; Break
  }
  Default
  {
    $qt_v6_recomended
  }
}



Switch ($Action)
{
  {$_ -eq "ListQT"}
  {

    $aqt_list_args = @("list-qt", "windows", "desktop" )

    $pinfo = New-Object System.Diagnostics.ProcessStartInfo
    $pinfo.FileName = $aqt_path
    $pinfo.UseShellExecute = $false
    $pinfo.Arguments = $aqt_list_args
    $p = New-Object System.Diagnostics.Process
    $p.StartInfo = $pinfo
    $p.Start() | Out-Null
    $p.WaitForExit()
  }

  {$_ -eq "ListArch"}
  {

    $aqt_list_arch_args = @("list-qt", "windows", "desktop", "--arch", $qt_ver )

    $pinfo = New-Object System.Diagnostics.ProcessStartInfo
    $pinfo.FileName = $aqt_path
    $pinfo.UseShellExecute = $false
    $pinfo.Arguments = $aqt_list_arch_args
    $p = New-Object System.Diagnostics.Process
    $p.StartInfo = $pinfo
    $p.Start() | Out-Null
    $p.WaitForExit()
  }

  {$_ -match "ListModules|InstallQT"}
  {

    $system_arch = (Get-CimInstance -Class Win32_ComputerSystem).SystemType
    if ($PSBoundParameters.ContainsKey('QtArch'))
    {
      $qt_arch =  $QtArch
    } else
    {
      Write-Host "Finding matching msvc install arch ..."

      $aqt_list_arch_args = @("list-qt", "windows", "desktop", "--arch", $qt_ver )

      $pinfo = New-Object System.Diagnostics.ProcessStartInfo
      $pinfo.FileName = $aqt_path
      $pinfo.RedirectStandardOutput = $true
      $pinfo.RedirectStandardError = $true
      $pinfo.UseShellExecute = $false
      $pinfo.Arguments = $aqt_list_arch_args
      $p = New-Object System.Diagnostics.Process
      $p.StartInfo = $pinfo
      $p.Start() | Out-Null
      $p.WaitForExit()
      $stdout = $p.StandardOutput.ReadToEnd()
      $arches = -split $stdout 
      $qt_arch =  Switch ($system_arch)
      {
        {$_ -match "x64" }
        { 
          $arch = $arches | Where-Object {$_ -match "win64_msvc\d+_64"} | Measure-Object -Maximum | Select-Object -First 1 -ExpandProperty Maximum
          Write-Host "Using ${arch}"
          $arch
        }
        {$_ -match "ARM" }
        { 
          $arch = $arches | Where-Object {$_ -match "win64_msvc\d+_arm64"} | Measure-Object -Maximum | Select-Object -First 1 -ExpandProperty Maximum
          Write-Host "Using ${arch}"
          $arch
        }
        default
        { 
          throw "System architecture unsupported by this script." 
        }
      }
    }

    if ( $Action -eq "ListModules")
    {
      $aqt_list_modules_args = @("list-qt", "windows", "desktop", "--modules", $qt_ver, $qt_arch )

      $pinfo = New-Object System.Diagnostics.ProcessStartInfo
      $pinfo.FileName = $aqt_path
      $pinfo.UseShellExecute = $false
      $pinfo.Arguments = $aqt_list_modules_args
      $p = New-Object System.Diagnostics.Process
      $p.StartInfo = $pinfo
      $p.Start() | Out-Null
      $p.WaitForExit()

    } else
    {

      Switch ($qt_ver)
      {
        {$_ -match "^5"}
        {
          $modules = @( "qtnetworkauth" )
          if ($IncludeDefultDebugInfoModules)
          {
            $modules += @( "debug_info" )
          }
        }
        default
        {
          $modules = @( "qtimageformats", "qt5compat", "qtnetworkauth" )
          if($IncludeDefultDebugInfoModules)
          {
            $modules += @( "qtimageformats.debug_information", "qt5compat.debug_information", "qtnetworkauth.debug_information" )
          }
        }
      }
      $modules += $AdtionalModules

      $aqt_install_args = @("install-qt", "windows", "desktop", $qt_ver, $qt_arch, "--outputdir", $qt_path, "-m"; $modules )
      $arch_dest = $qt_arch -replace "win64_"
      Write-Host "Installing Qt ${qt_ver}:${qt_arch} at `"Qt\${qt_ver}\${arch_dest}`""

      Write-Host "Running: aqt ${aqt_install_args}"

      $pinfo = New-Object System.Diagnostics.ProcessStartInfo
      $pinfo.FileName = $aqt_path
      $pinfo.UseShellExecute = $false
      $pinfo.Arguments = $aqt_install_args
      $p = New-Object System.Diagnostics.Process
      $p.StartInfo = $pinfo
      $p.Start() | Out-Null
      $p.WaitForExit()
    }

  }
}
