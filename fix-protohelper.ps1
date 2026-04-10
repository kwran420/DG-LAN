<#
.SYNOPSIS
  Migrates ProtoHelper::setStr calls from set_ to mutable_ protobuf API.

.DESCRIPTION
  Replaces `::set_<field>` with `::mutable_<field>` in ProtoHelper::setStr calls
  across all .cpp and .h files under application/.
  This handles the protobuf v3.x → v4.x setter API change.

.PARAMETER DryRun
  Show what would change without writing files.
#>
param(
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
$appDir = Join-Path $PSScriptRoot "application"

if (-not (Test-Path $appDir)) {
    Write-Error "Application directory not found: $appDir"
    exit 1
}

$files = Get-ChildItem -Path $appDir -Recurse -Include "*.cpp","*.h"
$changed = 0
foreach ($f in $files) {
    $lines = Get-Content $f.FullName
    $modified = $false
    $newLines = $lines | ForEach-Object {
        if ($_ -match 'ProtoHelper::setStr' -and $_ -match '&[\w:]+::set_') {
            $new = $_ -replace '(&[\w:]+)::set_(\w+)', '$1::mutable_$2'
            $modified = $true
            $new
        } else { $_ }
    }
    if ($modified) {
        if ($DryRun) {
            Write-Host "Would update: $($f.Name)" -ForegroundColor Yellow
        } else {
            Set-Content $f.FullName $newLines
            Write-Host "Updated: $($f.Name)" -ForegroundColor Green
        }
        $changed++
    }
}
Write-Host "Total files $( if ($DryRun) { 'to change' } else { 'changed' } ): $changed"
Write-Host "Total files changed: $changed"
