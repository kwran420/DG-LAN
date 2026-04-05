$files = Get-ChildItem -Path "C:\Users\kiera\OneDrive\Documents\DG-Lan\application" -Recurse -Include "*.cpp","*.h"
$changed = 0
foreach ($f in $files) {
    $content = [System.IO.File]::ReadAllText($f.FullName)
    $lines = $content -split "`n"
    $modified = $false
    $newLines = $lines | ForEach-Object {
        if ($_ -match 'ProtoHelper::setStr' -and $_ -match '&[\w:]+::set_') {
            $new = $_ -replace '(&[\w:]+)::set_(\w+)', '$1::mutable_$2'
            $modified = $true
            $new
        } else { $_ }
    }
    if ($modified) {
        $newContent = $newLines -join "`n"
        [System.IO.File]::WriteAllText($f.FullName, $newContent)
        $changed++
        Write-Host "Updated: $($f.Name)"
    }
}
Write-Host "Total files changed: $changed"
