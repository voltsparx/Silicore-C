[CmdletBinding()]
param(
  [switch]$NoInstall,
  [string]$Mode
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$ScriptName = Split-Path -Leaf $PSCommandPath
$ScriptDir = Split-Path -Parent $PSCommandPath
$RepoRoot = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $RepoRoot 'build'
$DefaultVcpkg = Join-Path $RepoRoot 'external\vcpkg'
$VcpkgRoot = if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { $DefaultVcpkg }

$DefaultInstallDir = 'C:\Program Files\silicore-c'
$BinName = 'silicore-c.exe'
$BinPath = Join-Path $BuildDir $BinName
$InstallDir = $null

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

function Prompt-YesNo {
  param(
    [string]$Message,
    [bool]$DefaultYes = $true
  )
  while ($true) {
    $suffix = if ($DefaultYes) { '[Y/n]' } else { '[y/N]' }
    $input = Read-Host "$Message $suffix"
    if ([string]::IsNullOrWhiteSpace($input)) {
      $input = if ($DefaultYes) { 'y' } else { 'n' }
    }
    switch ($input.ToLowerInvariant()) {
      'y' { return $true }
      'yes' { return $true }
      'n' { return $false }
      'no' { return $false }
      default { Write-Host 'Please answer yes or no.' }
    }
  }
}

function Prompt-Mode {
  if ($Mode) {
    return $Mode
  }
  Write-Host 'Select mode:'
  Write-Host '  1) Install'
  Write-Host '  2) Update'
  Write-Host '  3) Test'
  Write-Host '  4) Uninstall'
  $choice = Read-Host 'Enter choice'
  switch ($choice.ToLowerInvariant()) {
    '1' { return 'install' }
    'install' { return 'install' }
    '2' { return 'update' }
    'update' { return 'update' }
    '3' { return 'test' }
    'test' { return 'test' }
    '4' { return 'uninstall' }
    'uninstall' { return 'uninstall' }
    default { throw 'Invalid selection.' }
  }
}

function Find-InstallDir {
  $cmd = Get-Command silicore-c -ErrorAction SilentlyContinue
  if ($cmd) {
    return (Split-Path -Parent $cmd.Path)
  }
  $candidate = Join-Path $DefaultInstallDir $BinName
  if (Test-Path $candidate) {
    return $DefaultInstallDir
  }
  return $null
}

function Ensure-PathEntry {
  param([string]$Dir)
  $userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
  if ([string]::IsNullOrWhiteSpace($userPath)) {
    $userPath = ''
  }
  if ($userPath -notmatch [Regex]::Escape($Dir)) {
    $newPath = if ([string]::IsNullOrWhiteSpace($userPath)) { $Dir } else { "$userPath;$Dir" }
    [Environment]::SetEnvironmentVariable('Path', $newPath, 'User')
    $env:Path = "$env:Path;$Dir"
  }
}

function Remove-PathEntry {
  param([string]$Dir)
  $userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
  if ([string]::IsNullOrWhiteSpace($userPath)) {
    return
  }
  $parts = $userPath -split ';' | Where-Object { $_ -and ($_ -ne $Dir) }
  $newPath = ($parts -join ';')
  [Environment]::SetEnvironmentVariable('Path', $newPath, 'User')
}

function Resolve-Generator {
  $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
  if (Test-Path $vswhere) {
    return 'Visual Studio 17 2022'
  }
  $hasNinja = Get-Command ninja -ErrorAction SilentlyContinue
  $hasClang = Get-Command clang -ErrorAction SilentlyContinue
  $hasGcc = Get-Command gcc -ErrorAction SilentlyContinue
  if ($hasNinja -and ($hasClang -or $hasGcc)) {
    return 'Ninja'
  }
  return $null
}

function Install-Dependencies {
  if ($NoInstall) {
    throw 'Missing dependencies. Install CMake and Git or remove -NoInstall.'
  }

  if (Get-Command winget -ErrorAction SilentlyContinue) {
    Write-InfoLine 'Installing CMake and Git with winget...'
    & winget install -e --id Kitware.CMake --accept-source-agreements --accept-package-agreements
    & winget install -e --id Git.Git --accept-source-agreements --accept-package-agreements
    & winget install -e --id Ninja-build.Ninja --accept-source-agreements --accept-package-agreements
    & winget install -e --id Microsoft.VisualStudio.2022.BuildTools --accept-source-agreements --accept-package-agreements
    return
  }

  if (Get-Command choco -ErrorAction SilentlyContinue) {
    Write-InfoLine 'Installing CMake and Git with choco...'
    & choco install -y cmake git
    & choco install -y ninja visualstudio2022buildtools
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
  $generator = Resolve-Generator
  if (-not $generator) {
    throw 'No supported generator found. Install Visual Studio Build Tools or Ninja + LLVM/MinGW.'
  }
  $genArgs = @('-G', $generator)
  if ($generator -like 'Visual Studio*') {
    $genArgs += @('-A', 'x64')
  }
  & cmake -S $RepoRoot -B $BuildDir @genArgs -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DCMAKE_TOOLCHAIN_FILE="$(Join-Path $VcpkgRoot 'scripts\buildsystems\vcpkg.cmake')"
}

function Build-Project {
  & cmake --build $BuildDir --config Release
}

function Build-Binary {
  $total = 4
  $step = 1

  Write-ProgressLine -Step $step -Total $total -Label 'Checking dependencies'
  if (-not (Get-Command cmake -ErrorAction SilentlyContinue) -or -not (Get-Command git -ErrorAction SilentlyContinue)) {
    Install-Dependencies
  }
  if (-not (Resolve-Generator)) {
    if ($NoInstall) {
      throw 'No supported generator found. Install Visual Studio Build Tools or Ninja + LLVM/MinGW.'
    }
    Install-Dependencies
  }

  $step++
  Write-ProgressLine -Step $step -Total $total -Label 'Preparing vcpkg'
  Ensure-Vcpkg

  $step++
  Write-ProgressLine -Step $step -Total $total -Label 'Installing vcpkg ports'
  Ensure-VcpkgPorts

  $step++
  Write-ProgressLine -Step $step -Total $total -Label 'Configuring and building'
  Configure-Build
  Build-Project

  if (-not (Test-Path $BinPath)) {
    throw "Build failed: $BinPath not found."
  }
}

function Install-Binary {
  New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
  Copy-Item -Force $BinPath (Join-Path $InstallDir $BinName)
  Ensure-PathEntry $InstallDir
}

function Mode-Install {
  $existing = Find-InstallDir
  if ($existing) {
    Write-WarnLine "Silicore-C already installed at $existing."
    if (-not (Prompt-YesNo 'Overwrite existing installation?' $false)) {
      Write-InfoLine 'Installation aborted.'
      return
    }
  }

  if (Prompt-YesNo "Install to default ($DefaultInstallDir)?" $true) {
    $script:InstallDir = $DefaultInstallDir
  } else {
    $custom = Read-Host 'Enter custom install path'
    $script:InstallDir = if ([string]::IsNullOrWhiteSpace($custom)) { $DefaultInstallDir } else { $custom }
  }

  Build-Binary
  Install-Binary
  Write-InfoLine "Installed Silicore-C to $InstallDir"
}

function Mode-Update {
  $existing = Find-InstallDir
  if (-not $existing) {
    Write-WarnLine 'Silicore-C not found.'
    if (Prompt-YesNo 'Install it now?' $true) {
      Mode-Install
    }
    return
  }
  $script:InstallDir = $existing
  Build-Binary
  Install-Binary
  Write-InfoLine "Updated Silicore-C at $InstallDir"
}

function Mode-Test {
  Build-Binary
  Write-InfoLine "Test build ready. Binary: $BinPath"
}

function Mode-Uninstall {
  $existing = Find-InstallDir
  if (-not $existing) {
    Write-WarnLine 'Silicore-C not installed.'
    return
  }
  $script:InstallDir = $existing
  if (-not (Prompt-YesNo "Uninstall Silicore-C from $InstallDir?" $false)) {
    Write-InfoLine 'Uninstall aborted.'
    return
  }
  $target = Join-Path $InstallDir $BinName
  if (Test-Path $target) {
    Remove-Item -Force $target
  }
  if (Test-Path $InstallDir) {
    Remove-Item -Recurse -Force $InstallDir
  }
  Remove-PathEntry $InstallDir
  Write-InfoLine 'Uninstalled Silicore-C.'
}

try {
  $selected = (Prompt-Mode).ToLowerInvariant()
  switch ($selected) {
    'install' { Mode-Install }
    'update' { Mode-Update }
    'test' { Mode-Test }
    'uninstall' { Mode-Uninstall }
    default { throw "Unknown mode: $selected" }
  }
} catch {
  Write-ErrorLine $_.Exception.Message
  exit 1
}
