[CmdletBinding()]
param(
  [switch]$NoInstall
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$ScriptName = Split-Path -Leaf $PSCommandPath
$ScriptDir = Split-Path -Parent $PSCommandPath
$RepoRoot = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $RepoRoot 'build'
$DefaultVcpkg = Join-Path $RepoRoot 'external\vcpkg'
$VcpkgRoot = if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { $DefaultVcpkg }

function Write-InfoLine {
  param([string]$Message)
  Write-Host "[INFO] $Message"
}

function Write-WarnLine {
  param([string]$Message)
  Write-Warning $Message
}

function Write-ErrorLine {
  param([string]$Message)
  Write-Host "[ERROR] $Message" -ForegroundColor Red
}

function Write-ProgressLine {
  param(
    [int]$Step,
    [int]$Total,
    [string]$Label
  )
  $width = 24
  $filled = [Math]::Floor(($Step * $width) / [Math]::Max(1, $Total))
  if ($filled -gt 0) {
    $bar = ('=' * ($filled - 1)) + '>'
  } else {
    $bar = '>'
  }
  $pad = ' ' * ($width - $filled)
  Write-Host ("[{0}{1}] {2}" -f $bar, $pad, $Label)
}

function Install-Dependencies {
  if ($NoInstall) {
    throw 'Missing dependencies. Install CMake and Git or remove -NoInstall.'
  }

  if (Get-Command winget -ErrorAction SilentlyContinue) {
    Write-InfoLine 'Installing CMake and Git with winget...'
    & winget install -e --id Kitware.CMake --accept-source-agreements --accept-package-agreements
    & winget install -e --id Git.Git --accept-source-agreements --accept-package-agreements
    return
  }

  if (Get-Command choco -ErrorAction SilentlyContinue) {
    Write-InfoLine 'Installing CMake and Git with choco...'
    & choco install -y cmake git
    return
  }

  throw 'Neither winget nor choco is available. Install CMake and Git manually.'
}

function Ensure-Vcpkg {
  if (Test-Path $VcpkgRoot) {
    Write-InfoLine "Using vcpkg at $VcpkgRoot"
    return
  }
  if ($NoInstall) {
    throw 'vcpkg not found. Set VCPKG_ROOT or remove -NoInstall to bootstrap.'
  }
  Write-InfoLine "Cloning vcpkg into $VcpkgRoot"
  New-Item -ItemType Directory -Force -Path (Split-Path -Parent $VcpkgRoot) | Out-Null
  git clone https://github.com/microsoft/vcpkg.git $VcpkgRoot
  & (Join-Path $VcpkgRoot 'bootstrap-vcpkg.bat')
}

function Ensure-VcpkgPorts {
  & (Join-Path $VcpkgRoot 'vcpkg') install curl openssl nlohmann-json
}

function Configure-Build {
  & cmake -S $RepoRoot -B $BuildDir -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DCMAKE_TOOLCHAIN_FILE="$(Join-Path $VcpkgRoot 'scripts\buildsystems\vcpkg.cmake')"
}

function Build-Project {
  & cmake --build $BuildDir --config Release
}

try {
  $total = 6
  $step = 1

  Write-ProgressLine -Step $step -Total $total -Label 'Checking dependencies'
  if (-not (Get-Command cmake -ErrorAction SilentlyContinue) -or -not (Get-Command git -ErrorAction SilentlyContinue)) {
    Install-Dependencies
  }

  $step++
  Write-ProgressLine -Step $step -Total $total -Label 'Preparing vcpkg'
  Ensure-Vcpkg

  $step++
  Write-ProgressLine -Step $step -Total $total -Label 'Installing vcpkg ports'
  Ensure-VcpkgPorts

  $step++
  Write-ProgressLine -Step $step -Total $total -Label 'Configuring build'
  Configure-Build

  $step++
  Write-ProgressLine -Step $step -Total $total -Label 'Building Silicore-C'
  Build-Project

  $step++
  Write-ProgressLine -Step $step -Total $total -Label 'Done'
  Write-InfoLine "Build completed. Binary: $BuildDir\silicore-c.exe"
} catch {
  Write-ErrorLine $_.Exception.Message
  exit 1
}
