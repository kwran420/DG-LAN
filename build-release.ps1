<#
.SYNOPSIS
  Build DG-LAN installer locally. Much faster than waiting for GitHub Actions.

.DESCRIPTION
  Builds Core + GUI, patches Version.h with build time/git hash,
  then produces the Inno Setup installer .exe.

  By default, also commits, tags, pushes, and creates a GitHub Release
  with the installer attached. Clients auto-update from there.
  Use -SkipPublish to build locally without pushing.

.EXAMPLE
  .\build-release.ps1                        # build + publish (default)
  .\build-release.ps1 -Version 2.0.0         # override version + publish
  .\build-release.ps1 -SkipBuild             # just rebuild the installer + publish
  .\build-release.ps1 -SkipPublish           # build only, no push
#>
param(
    [string]$Version,
    [switch]$SkipBuild,
    [switch]$SkipPublish
)

$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot

# ── Paths ─────────────────────────────────────────────────────
$msys2  = "C:\msys64"
$mingw  = "$msys2\mingw64"
$bash   = "$msys2\usr\bin\bash.exe"
$iscc   = "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"
if (-not (Test-Path $iscc)) {
    $iscc = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
}

# Ensure MSYS2 bash sees the MinGW64 toolchain
$env:MSYSTEM = "MINGW64"

foreach ($tool in @($bash, "$mingw\bin\Qt5Core.dll")) {
    if (-not (Test-Path $tool)) {
        Write-Error "Required tool not found: $tool"
        exit 1
    }
}

# ── Update Version.h ─────────────────────────────────────────
$versionFile = "application\Common\Version.h"
$content = Get-Content $versionFile -Raw
$bt   = [DateTime]::UtcNow.ToString("yyyy-MM-dd_HH-mm")
$hash = (git rev-parse HEAD).Substring(0, 12)

if ($Version) {
    $content = $content -replace '#define VERSION "[^"]*"', "#define VERSION `"$Version`""
    Write-Host "Version set to: $Version" -ForegroundColor Cyan
} elseif ($content -match '#define VERSION "(\d+)\.(\d+)\.(\d+)"') {
    $major = [int]$Matches[1]
    $minor = [int]$Matches[2]
    $patch = [int]$Matches[3] + 1
    $Version = "$major.$minor.$patch"
    $content = $content -replace '#define VERSION "[^"]*"', "#define VERSION `"$Version`""
    Write-Host "Version auto-incremented to: $Version" -ForegroundColor Cyan
} elseif ($content -match '#define VERSION "([^"]+)"') {
    $Version = $Matches[1]
    Write-Host "Version from Version.h (no auto-increment): $Version" -ForegroundColor Yellow
}

$content = $content -replace '#define BUILD_TIME "[^"]*"',  "#define BUILD_TIME `"$bt`""
$content = $content -replace '#define GIT_VERSION "[^"]*"', "#define GIT_VERSION `"$hash`""
Set-Content $versionFile $content -NoNewline

Write-Host "Build time: $bt  Git: $hash" -ForegroundColor DarkGray

# ── Build Core + GUI ─────────────────────────────────────────
if (-not $SkipBuild) {
    # Convert Windows path to MSYS2 path (handles spaces)
    $winPath = $PWD.Path -replace '\\','/'
    if ($winPath -match '^([A-Z]):(.*)$') {
        $appDir = "/$($Matches[1].ToLower())$($Matches[2])"
    } else {
        $appDir = $winPath
    }

    $buildCore = @"
cd '$appDir/application' && QMAKE=`$(command -v qmake-qt5 2>/dev/null || command -v qmake) && `$QMAKE Core.pro -r -spec win32-g++ 'CONFIG+=release' && mingw32-make -f Makefile-Core -j`$(nproc)
"@
    $buildGUI = @"
cd '$appDir/application' && QMAKE=`$(command -v qmake-qt5 2>/dev/null || command -v qmake) && `$QMAKE GUI.pro -r -spec win32-g++ 'CONFIG+=release' && mingw32-make -f Makefile-GUI -j`$(nproc)
"@

    Write-Host "`n=== Building Core ===" -ForegroundColor Green
    & $bash --login -c $buildCore
    if ($LASTEXITCODE -ne 0) { Write-Error "Core build failed"; exit 1 }

    Write-Host "`n=== Building GUI ===" -ForegroundColor Green
    & $bash --login -c $buildGUI
    if ($LASTEXITCODE -ne 0) { Write-Error "GUI build failed"; exit 1 }
}

# ── Verify build outputs ─────────────────────────────────────
$coreExe = "application\Core\output\release\DG-LAN.Core.exe"
$guiExe  = "application\GUI\output\release\DG-LAN.GUI.exe"
foreach ($exe in @($coreExe, $guiExe)) {
    if (-not (Test-Path $exe)) { Write-Error "Build output missing: $exe"; exit 1 }
}
Write-Host "Core: $coreExe" -ForegroundColor DarkGray
Write-Host "GUI:  $guiExe" -ForegroundColor DarkGray

# ── Build installer ──────────────────────────────────────────
Write-Host "`n=== Building Installer ===" -ForegroundColor Green
$installerLog = Join-Path $PSScriptRoot "installer_log.txt"
Push-Location application\Setups\Windows
try {
    & $iscc windows_setup.iss *> $installerLog
    $isccExit = $LASTEXITCODE
    # Show only summary lines (no individual file paths that render as images)
    $logLines = Get-Content $installerLog
    $logLines | Where-Object { $_ -match '^\s*(Compiling|Compression|Output|Successful)' } |
        ForEach-Object { Write-Host $_ -ForegroundColor DarkGray }
    if ($isccExit -ne 0) {
        Write-Host "Full log: $installerLog" -ForegroundColor Yellow
        Write-Error "Inno Setup failed (exit code $isccExit)"
        exit 1
    }
} finally {
    Pop-Location
}

# ── Done ──────────────────────────────────────────────────────
$installer = Get-ChildItem "application\Setups\Windows\Installations\*.exe" |
             Sort-Object LastWriteTime -Descending | Select-Object -First 1
Write-Host "`n=== SUCCESS ===" -ForegroundColor Green
Write-Host "Installer: $($installer.FullName)" -ForegroundColor Cyan
Write-Host "Size: $([math]::Round($installer.Length / 1MB, 1)) MB"

# ── Publish to GitHub ─────────────────────────────────────────
if (-not $SkipPublish) {
    $tag = "v$Version"
    Write-Host "`n=== Publishing $tag ===" -ForegroundColor Green

    # Ensure gh CLI is available
    if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
        Write-Error "GitHub CLI (gh) not found. Install from https://cli.github.com"
        exit 1
    }

    # Read VERSION_TAG for the release title
    $vhContent = Get-Content $versionFile -Raw
    $versionTag = ""
    if ($vhContent -match '#define VERSION_TAG "([^"]*)"') { $versionTag = $Matches[1] }

    # Commit any pending changes (Version.h build time, etc.)
    git add -A
    $commitMsg = "chore: release $tag"
    git diff --cached --quiet 2>$null
    if ($LASTEXITCODE -ne 0) {
        git commit -m $commitMsg
        Write-Host "Committed: $commitMsg" -ForegroundColor DarkGray
    }

    # Create tag (skip if it already exists)
    $existingTag = git tag -l $tag
    if ($existingTag) {
        Write-Host "Tag $tag already exists — skipping tag creation" -ForegroundColor Yellow
    } else {
        git tag $tag
        Write-Host "Tagged: $tag" -ForegroundColor DarkGray
    }

    # Push
    git push origin master
    git push origin $tag
    Write-Host "Pushed to origin" -ForegroundColor DarkGray

    # Create GitHub Release with the installer attached
    $title = "DG-LAN $Version $versionTag".Trim()
    $notes = "Version: $Version $versionTag`nBuild: $bt"

    # Delete existing release if present (allows re-publish)
    gh release view $tag 2>$null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Release $tag already exists — replacing asset" -ForegroundColor Yellow
        gh release delete $tag --yes 2>$null
    }

    gh release create $tag $($installer.FullName) `
        --title $title `
        --notes $notes `
        --latest

    if ($LASTEXITCODE -eq 0) {
        Write-Host "`n=== PUBLISHED ===" -ForegroundColor Green
        Write-Host "https://github.com/kwran420/DG-LAN/releases/tag/$tag" -ForegroundColor Cyan
    } else {
        Write-Error "Failed to create GitHub release"
        exit 1
    }
}
