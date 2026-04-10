Add-Type -AssemblyName System.Drawing

$root     = Split-Path -Parent $MyInvocation.MyCommand.Definition
$logoPath = Join-Path $root "application\GUI\ressources\logo.png"
$icoPath  = Join-Path $root "application\Common\ressources\icon.ico"
$iconPng  = Join-Path $root "application\GUI\ressources\icon.png"
$logPath  = Join-Path $root "gen_icons_log.txt"

# Visual padding: 90% of available space, leaving a border around the icon
$iconPadding = 0.9

Set-Content $logPath "Starting icon generation..."

if (-not (Test-Path $logoPath)) {
    Write-Error "Logo not found: $logoPath"
    exit 1
}

$src = $null
$bitmaps = @()
$icoStream = $null
$icoWriter = $null

try {
    $src = [System.Drawing.Bitmap]::FromFile($logoPath)
    Add-Content $logPath "Logo loaded: $($src.Width)x$($src.Height)"

    # ---- Helper: Create a scaled bitmap from source ----
    function New-ScaledBitmap($source, $targetSize, $scale) {
        $bitmap = New-Object System.Drawing.Bitmap $targetSize, $targetSize
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        try {
            $graphics.Clear([System.Drawing.Color]::Transparent)
            $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $ratio = [Math]::Min($targetSize / $source.Width, $targetSize / $source.Height)
            $scaledWidth  = [int]($source.Width  * $ratio * $scale)
            $scaledHeight = [int]($source.Height * $ratio * $scale)
            $offsetX = [int](($targetSize - $scaledWidth)  / 2)
            $offsetY = [int](($targetSize - $scaledHeight) / 2)
            $graphics.DrawImage($source, $offsetX, $offsetY, $scaledWidth, $scaledHeight)
        } finally {
            $graphics.Dispose()
        }
        return $bitmap
    }

    # ---- Create icon.png (256x256) ----
    $iconSize = 256
    $bmpPng = New-ScaledBitmap $src $iconSize $iconPadding
    $bmpPng.Save($iconPng, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmpPng.Dispose()
    Add-Content $logPath "icon.png saved OK"

    # ---- Create multi-size .ico ----
    $sizes = @(256, 128, 64, 48, 32, 16)
    foreach ($sz in $sizes) {
        $bitmaps += New-ScaledBitmap $src $sz $iconPadding
    }

    # Write ICO file manually (ICO format)
    $icoStream = New-Object System.IO.MemoryStream
    $icoWriter = New-Object System.IO.BinaryWriter($icoStream)

    # ICONDIR header
    $icoWriter.Write([uint16]0)             # reserved
    $icoWriter.Write([uint16]1)             # type: 1=icon
    $icoWriter.Write([uint16]$sizes.Count)  # count

    # Collect PNG-encoded images
    $pngDatas = @()
    foreach ($bmp in $bitmaps) {
        $pngStream = New-Object System.IO.MemoryStream
        $bmp.Save($pngStream, [System.Drawing.Imaging.ImageFormat]::Png)
        $pngDatas += ,$pngStream.ToArray()
        $pngStream.Dispose()
    }

    # Calculate offsets: header(6) + N*16 dir entries
    $offset = 6 + $sizes.Count * 16
    foreach ($i in 0..($sizes.Count-1)) {
        $sz = $sizes[$i]
        $szByte = if ($sz -ge 256) { 0 } else { $sz }
        $dataLen = $pngDatas[$i].Length
        $icoWriter.Write([byte]$szByte)    # width
        $icoWriter.Write([byte]$szByte)    # height
        $icoWriter.Write([byte]0)          # color count
        $icoWriter.Write([byte]0)          # reserved
        $icoWriter.Write([uint16]1)        # planes
        $icoWriter.Write([uint16]32)       # bit count
        $icoWriter.Write([uint32]$dataLen) # data size
        $icoWriter.Write([uint32]$offset)  # offset
        $offset += $dataLen
    }
    foreach ($data in $pngDatas) {
        $icoWriter.Write($data)
    }
    $icoWriter.Flush()
    [System.IO.File]::WriteAllBytes($icoPath, $icoStream.ToArray())

    Add-Content $logPath "icon.ico saved OK ($($sizes.Count) sizes)"
    Add-Content $logPath "Done."
} finally {
    foreach ($bmp in $bitmaps) { if ($bmp) { $bmp.Dispose() } }
    if ($icoWriter) { $icoWriter.Dispose() }
    if ($icoStream) { $icoStream.Dispose() }
    if ($src) { $src.Dispose() }
}
