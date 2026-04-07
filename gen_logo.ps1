Add-Type -AssemblyName System.Drawing

$root     = Split-Path -Parent $MyInvocation.MyCommand.Definition
$logoPath = Join-Path $root "application\GUI\ressources\logo.png"

$sz = 512
$bmp = New-Object System.Drawing.Bitmap $sz, $sz
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
$g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
$g.Clear([System.Drawing.Color]::Transparent)

# ── Background circle ──────────────────────────────────────────────────────
$pad = 16
$circBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
    (New-Object System.Drawing.Point 0, 0),
    (New-Object System.Drawing.Point 0, $sz),
    [System.Drawing.Color]::FromArgb(255, 18, 42, 90),
    [System.Drawing.Color]::FromArgb(255, 10, 24, 56)
)
$g.FillEllipse($circBrush, $pad, $pad, $sz - 2*$pad, $sz - 2*$pad)

# Subtle ring highlight
$ringPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(80, 100, 170, 255), 3)
$g.DrawEllipse($ringPen, $pad, $pad, $sz - 2*$pad, $sz - 2*$pad)

# ── RJ45 Connector ────────────────────────────────────────────────────────
$cx = $sz / 2
$cy = $sz * 0.36

# Connector body
$bodyW = 140; $bodyH = 100
$bodyBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
    (New-Object System.Drawing.Point ($cx - $bodyW/2), ($cy - $bodyH/2)),
    (New-Object System.Drawing.Point ($cx - $bodyW/2), ($cy + $bodyH/2)),
    [System.Drawing.Color]::FromArgb(255, 200, 210, 225),
    [System.Drawing.Color]::FromArgb(255, 150, 160, 180)
)
$bodyRect = New-Object System.Drawing.RectangleF(($cx - $bodyW/2), ($cy - $bodyH/2), $bodyW, $bodyH)
$g.FillRectangle($bodyBrush, $bodyRect)

# Clip/latch on top
$clipW = 60; $clipH = 30
$clipBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 170, 185, 205))
$clipRect = New-Object System.Drawing.RectangleF(($cx - $clipW/2), ($cy - $bodyH/2 - $clipH + 6), $clipW, $clipH)
$g.FillRectangle($clipBrush, $clipRect)

# Face/socket (dark inset at bottom of connector body)
$faceW = 120; $faceH = 35
$faceBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 20, 30, 55))
$faceRect = New-Object System.Drawing.RectangleF(($cx - $faceW/2), ($cy + $bodyH/2 - $faceH - 8), $faceW, $faceH)
$g.FillRectangle($faceBrush, $faceRect)

# Gold contact pins (8 pins for RJ45)
$pinPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 255, 215, 50), 2.5)
$pinCount = 8
$pinStartX = $cx - $faceW/2 + 12
$pinSpacing = ($faceW - 24) / ($pinCount - 1)
$pinTop = $cy + $bodyH/2 - $faceH - 4
$pinBot = $cy + $bodyH/2 - 12
for ($i = 0; $i -lt $pinCount; $i++) {
    $px = $pinStartX + $i * $pinSpacing
    $g.DrawLine($pinPen, [float]$px, [float]$pinTop, [float]$px, [float]$pinBot)
}

# Cable coming out top
$cablePen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 80, 100, 140), 12)
$g.DrawLine($cablePen, [float]$cx, [float]($cy - $bodyH/2 - $clipH + 6), [float]$cx, [float]($cy - $bodyH/2 - 50))

# ── "DG-LAN" text below connector ─────────────────────────────────────────
$textY = $cy + $bodyH/2 + 30
$fontFamily = New-Object System.Drawing.FontFamily("Arial")
$textFont = New-Object System.Drawing.Font($fontFamily, 72, [System.Drawing.FontStyle]::Bold)
$textBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 220, 235, 255))

$sf = New-Object System.Drawing.StringFormat
$sf.Alignment = [System.Drawing.StringAlignment]::Center
$sf.LineAlignment = [System.Drawing.StringAlignment]::Near

$textRect = New-Object System.Drawing.RectangleF(0, $textY, $sz, 120)
$g.DrawString("DG-LAN", $textFont, $textBrush, $textRect, $sf)

# Subtle glow under text
$glowBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(40, 100, 170, 255))
$g.FillEllipse($glowBrush, ($cx - 120), ($textY + 70), 240, 20)

$g.Dispose()
$bmp.Save($logoPath, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()

Write-Host "Logo generated: $logoPath"
