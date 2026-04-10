Add-Type -AssemblyName System.Drawing

$root     = Split-Path -Parent $MyInvocation.MyCommand.Definition
$logoPath = Join-Path $root "application\GUI\ressources\logo.png"

# ── Layout constants ──────────────────────────────────────────────────────
$logoSize      = 512
$circlePadding = 16
$pinCount      = 8     # RJ45 has 8 contact pins
$fontSize      = 72
$fontName      = "Arial"

# ── Color palette ─────────────────────────────────────────────────────────
$bgGradientStart  = [System.Drawing.Color]::FromArgb(255, 18, 42, 90)
$bgGradientEnd    = [System.Drawing.Color]::FromArgb(255, 10, 24, 56)
$ringHighlight    = [System.Drawing.Color]::FromArgb(80, 100, 170, 255)
$connectorTop     = [System.Drawing.Color]::FromArgb(255, 200, 210, 225)
$connectorBottom  = [System.Drawing.Color]::FromArgb(255, 150, 160, 180)
$clipColor        = [System.Drawing.Color]::FromArgb(255, 170, 185, 205)
$socketColor      = [System.Drawing.Color]::FromArgb(255, 20, 30, 55)
$pinGold          = [System.Drawing.Color]::FromArgb(255, 255, 215, 50)
$cableColor       = [System.Drawing.Color]::FromArgb(255, 80, 100, 140)
$textColor        = [System.Drawing.Color]::FromArgb(255, 220, 235, 255)
$glowColor        = [System.Drawing.Color]::FromArgb(40, 100, 170, 255)

# ── Connector dimensions ─────────────────────────────────────────────────
$bodyW = 140; $bodyH = 100
$clipW = 60;  $clipH = 30
$faceW = 120; $faceH = 35

$disposables = [System.Collections.Generic.List[System.IDisposable]]::new()
try {
    $bmp = New-Object System.Drawing.Bitmap $logoSize, $logoSize
    $disposables.Add($bmp)
    $graphics = [System.Drawing.Graphics]::FromImage($bmp)
    $disposables.Add($graphics)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
    $graphics.Clear([System.Drawing.Color]::Transparent)

    # ── Background circle ──────────────────────────────────────────────────
    $circBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
        (New-Object System.Drawing.Point 0, 0),
        (New-Object System.Drawing.Point 0, $logoSize),
        $bgGradientStart, $bgGradientEnd
    )
    $disposables.Add($circBrush)
    $circleSize = $logoSize - 2 * $circlePadding
    $graphics.FillEllipse($circBrush, $circlePadding, $circlePadding, $circleSize, $circleSize)

    $ringPen = New-Object System.Drawing.Pen($ringHighlight, 3)
    $disposables.Add($ringPen)
    $graphics.DrawEllipse($ringPen, $circlePadding, $circlePadding, $circleSize, $circleSize)

    # ── RJ45 Connector ────────────────────────────────────────────────────
    $cx = $logoSize / 2
    $cy = $logoSize * 0.36

    # Connector body
    $bodyBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
        (New-Object System.Drawing.Point ($cx - $bodyW/2), ($cy - $bodyH/2)),
        (New-Object System.Drawing.Point ($cx - $bodyW/2), ($cy + $bodyH/2)),
        $connectorTop, $connectorBottom
    )
    $disposables.Add($bodyBrush)
    $bodyRect = New-Object System.Drawing.RectangleF(($cx - $bodyW/2), ($cy - $bodyH/2), $bodyW, $bodyH)
    $graphics.FillRectangle($bodyBrush, $bodyRect)

    # Clip/latch on top
    $latchBrush = New-Object System.Drawing.SolidBrush($clipColor)
    $disposables.Add($latchBrush)
    $clipRect = New-Object System.Drawing.RectangleF(($cx - $clipW/2), ($cy - $bodyH/2 - $clipH + 6), $clipW, $clipH)
    $graphics.FillRectangle($latchBrush, $clipRect)

    # Face/socket (dark inset at bottom of connector body)
    $faceBrush = New-Object System.Drawing.SolidBrush($socketColor)
    $disposables.Add($faceBrush)
    $faceRect = New-Object System.Drawing.RectangleF(($cx - $faceW/2), ($cy + $bodyH/2 - $faceH - 8), $faceW, $faceH)
    $graphics.FillRectangle($faceBrush, $faceRect)

    # Gold contact pins
    $pinPen = New-Object System.Drawing.Pen($pinGold, 2.5)
    $disposables.Add($pinPen)
    $pinStartX = $cx - $faceW/2 + 12
    $pinSpacing = ($faceW - 24) / ($pinCount - 1)
    $pinTop = $cy + $bodyH/2 - $faceH - 4
    $pinBot = $cy + $bodyH/2 - 12
    for ($i = 0; $i -lt $pinCount; $i++) {
        $px = $pinStartX + $i * $pinSpacing
        $graphics.DrawLine($pinPen, [float]$px, [float]$pinTop, [float]$px, [float]$pinBot)
    }

    # Cable coming out top
    $cablePen = New-Object System.Drawing.Pen($cableColor, 12)
    $disposables.Add($cablePen)
    $graphics.DrawLine($cablePen, [float]$cx, [float]($cy - $bodyH/2 - $clipH + 6), [float]$cx, [float]($cy - $bodyH/2 - 50))

    # ── "DG-LAN" text below connector ──────────────────────────────────────
    $textY = $cy + $bodyH/2 + 30
    $fontFamily = New-Object System.Drawing.FontFamily($fontName)
    $disposables.Add($fontFamily)
    $textFont = New-Object System.Drawing.Font($fontFamily, $fontSize, [System.Drawing.FontStyle]::Bold)
    $disposables.Add($textFont)
    $textBrush = New-Object System.Drawing.SolidBrush($textColor)
    $disposables.Add($textBrush)

    $sf = New-Object System.Drawing.StringFormat
    $disposables.Add($sf)
    $sf.Alignment = [System.Drawing.StringAlignment]::Center
    $sf.LineAlignment = [System.Drawing.StringAlignment]::Near

    $textRect = New-Object System.Drawing.RectangleF(0, $textY, $logoSize, 120)
    $graphics.DrawString("DG-LAN", $textFont, $textBrush, $textRect, $sf)

    # Subtle glow under text
    $glowBrush = New-Object System.Drawing.SolidBrush($glowColor)
    $disposables.Add($glowBrush)
    $graphics.FillEllipse($glowBrush, ($cx - 120), ($textY + 70), 240, 20)

    $graphics.Dispose()
    $bmp.Save($logoPath, [System.Drawing.Imaging.ImageFormat]::Png)

    Write-Host "Logo generated: $logoPath"
} finally {
    foreach ($d in $disposables) {
        try { $d.Dispose() } catch { }
    }
}
