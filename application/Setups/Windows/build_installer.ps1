# build_installer.ps1 - Build Inno Setup installer with OneDrive hydration safety net
# This script ensures all workspace files are pinned locally before running ISCC,
# preventing builds from failing due to OneDrive "Files On-Demand" dehydration.

param(
    [switch]$SkipHydration
)

$ErrorActionPreference = 'Stop'
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$workspaceRoot = (Resolve-Path "$scriptDir\..\..\..").Path
$issFile = Join-Path $scriptDir "windows_setup.iss"
$iscc = "C:\Users\kiera\AppData\Local\Programs\Inno Setup 6\ISCC.exe"

if (-not (Test-Path $iscc)) {
    Write-Error "Inno Setup not found at: $iscc"
    exit 1
}

if (-not (Test-Path $issFile)) {
    Write-Error "ISS file not found at: $issFile"
    exit 1
}

# --- OneDrive hydration safety net ---
if (-not $SkipHydration) {
    Write-Host "[Hydrate] Pinning all workspace files (attrib +P) to prevent OneDrive dehydration..." -ForegroundColor Cyan
    attrib +P /S /D "$workspaceRoot\*" 2>$null

    # Force-read any cloud-only files to trigger download
    $cloudOnly = 0
    Get-ChildItem -Recurse -File $workspaceRoot -ErrorAction SilentlyContinue | ForEach-Object {
        $attr = $_.Attributes
        if ($attr -band [System.IO.FileAttributes]::Offline) {
            $cloudOnly++
            Write-Host "  Hydrating: $($_.FullName)" -ForegroundColor Yellow
            # Reading the file forces OneDrive to download it
            [void][System.IO.File]::ReadAllBytes($_.FullName)
        }
    }
    if ($cloudOnly -gt 0) {
        Write-Host "[Hydrate] Downloaded $cloudOnly cloud-only file(s)." -ForegroundColor Green
    } else {
        Write-Host "[Hydrate] All files already local." -ForegroundColor Green
    }
}

# --- Build installer ---
Write-Host "`n[Build] Running Inno Setup compiler..." -ForegroundColor Cyan
& $iscc $issFile
$exitCode = $LASTEXITCODE

if ($exitCode -eq 0) {
    Write-Host "`n[Build] Installer built successfully!" -ForegroundColor Green
    # Show the output file
    $installations = Join-Path $scriptDir "Installations"
    $latest = Get-ChildItem $installations -Filter "*.exe" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($latest) {
        Write-Host "  Output: $($latest.FullName)" -ForegroundColor Green
        Write-Host "  Size:   $([math]::Round($latest.Length / 1MB, 1)) MB" -ForegroundColor Green
    }
} else {
    Write-Host "`n[Build] FAILED with exit code $exitCode" -ForegroundColor Red
}

exit $exitCode
