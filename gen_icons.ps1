Add-Type -AssemblyName System.Drawing

$root     = Split-Path -Parent $MyInvocation.MyCommand.Definition
$logoPath = Join-Path $root "application\GUI\ressources\logo.png"
$icoPath  = Join-Path $root "application\Common\ressources\icon.ico"
$iconPng  = Join-Path $root "application\GUI\ressources\icon.png"
$logPath  = Join-Path $root "gen_icons_log.txt"

Set-Content $logPath "Starting icon generation..."

$src = [System.Drawing.Bitmap]::FromFile($logoPath)
Add-Content $logPath "Logo loaded: $($src.Width)x$($src.Height)"

# ---- Create icon.png (256x256, transparent/dark bg) ----
$sz256 = 256
$bmpPng = New-Object System.Drawing.Bitmap $sz256, $sz256
$g = [System.Drawing.Graphics]::FromImage($bmpPng)
$g.Clear([System.Drawing.Color]::Transparent)
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$ratio = [Math]::Min($sz256 / $src.Width, $sz256 / $src.Height)
$w = [int]($src.Width * $ratio * 0.9)
$h = [int]($src.Height * $ratio * 0.9)
$x = [int](($sz256 - $w) / 2)
$y = [int](($sz256 - $h) / 2)
$g.DrawImage($src, $x, $y, $w, $h)
$g.Dispose()
$bmpPng.Save($iconPng, [System.Drawing.Imaging.ImageFormat]::Png)
$bmpPng.Dispose()
Add-Content $logPath "icon.png saved OK"

# ---- Create multi-size .ico ----
$sizes = @(256, 128, 64, 48, 32, 16)
$bitmaps = @()
foreach ($sz in $sizes) {
    $bmp = New-Object System.Drawing.Bitmap $sz, $sz
    $g2 = [System.Drawing.Graphics]::FromImage($bmp)
    $g2.Clear([System.Drawing.Color]::Transparent)
    $g2.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $r = [Math]::Min($sz / $src.Width, $sz / $src.Height)
    $bw = [int]($src.Width * $r * 0.9)
    $bh = [int]($src.Height * $r * 0.9)
    $bx = [int](($sz - $bw) / 2)
    $by = [int](($sz - $bh) / 2)
    $g2.DrawImage($src, $bx, $by, $bw, $bh)
    $g2.Dispose()
    $bitmaps += $bmp
}
$src.Dispose()

# Write ICO file manually (ICO format)
$ms = New-Object System.IO.MemoryStream
$bw2 = New-Object System.IO.BinaryWriter($ms)

# ICONDIR header
$bw2.Write([uint16]0)          # reserved
$bw2.Write([uint16]1)          # type: 1=icon
$bw2.Write([uint16]$sizes.Count) # count

# Collect PNG-encoded images
$pngDatas = @()
foreach ($bmp in $bitmaps) {
    $pms = New-Object System.IO.MemoryStream
    $bmp.Save($pms, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    $pngDatas += ,$pms.ToArray()
    $pms.Dispose()
}

# Calculate offsets: header(6) + N*16 dir entries
$offset = 6 + $sizes.Count * 16
foreach ($i in 0..($sizes.Count-1)) {
    $sz = $sizes[$i]
    $szByte = if ($sz -ge 256) { 0 } else { $sz }
    $dataLen = $pngDatas[$i].Length
    $bw2.Write([byte]$szByte)   # width
    $bw2.Write([byte]$szByte)   # height
    $bw2.Write([byte]0)          # color count
    $bw2.Write([byte]0)          # reserved
    $bw2.Write([uint16]1)        # planes
    $bw2.Write([uint16]32)       # bit count
    $bw2.Write([uint32]$dataLen) # data size
    $bw2.Write([uint32]$offset)  # offset
    $offset += $dataLen
}
foreach ($data in $pngDatas) {
    $bw2.Write($data)
}
$bw2.Flush()
[System.IO.File]::WriteAllBytes($icoPath, $ms.ToArray())
$bw2.Dispose()
$ms.Dispose()

Add-Content $logPath "icon.ico saved OK ($($sizes.Count) sizes)"
Add-Content $logPath "Done."
