# Source for the original code-drawn icon (no downloaded artwork).
Add-Type -AssemblyName System.Drawing
$frames = @()
foreach ($size in @(16, 32, 48, 256)) {
    $bitmap = New-Object System.Drawing.Bitmap($size, $size)
    $g = [System.Drawing.Graphics]::FromImage($bitmap)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::Transparent)
    $bg = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 26, 86, 104))
    $fg = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 231, 249, 244))
    $accent = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 106, 224, 190))
    $s = $size / 32.0
    $g.FillEllipse($bg, [single](1*$s), [single](1*$s), [single](30*$s), [single](30*$s))
    foreach ($p in @(@(8,9), @(8,17), @(17,9))) { $g.FillRectangle($fg, [single]($p[0]*$s), [single]($p[1]*$s), [single](6*$s), [single](6*$s)) }
    $g.FillRectangle($accent, [single](17*$s), [single](17*$s), [single](6*$s), [single](6*$s))
    $stream = New-Object System.IO.MemoryStream
    $bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
    $frames += ,@($size, $stream.ToArray())
    $stream.Dispose(); $g.Dispose(); $bitmap.Dispose(); $bg.Dispose(); $fg.Dispose(); $accent.Dispose()
}
$path = Join-Path (Split-Path $PSScriptRoot -Parent) 'assets\deskh.ico'
$file = [System.IO.File]::Create($path)
$writer = New-Object System.IO.BinaryWriter($file)
$writer.Write([uint16]0); $writer.Write([uint16]1); $writer.Write([uint16]$frames.Count)
$offset = 6 + 16 * $frames.Count
foreach ($frame in $frames) {
    $dimension = if ($frame[0] -eq 256) { 0 } else { $frame[0] }
    $writer.Write([byte]$dimension); $writer.Write([byte]$dimension)
    $writer.Write([byte]0); $writer.Write([byte]0)
    $writer.Write([uint16]1); $writer.Write([uint16]32)
    $writer.Write([uint32]$frame[1].Length); $writer.Write([uint32]$offset)
    $offset += $frame[1].Length
}
foreach ($frame in $frames) { $writer.Write([byte[]]$frame[1]) }
$writer.Dispose(); $file.Dispose()
