# GameHost end-to-end check (DevOps GP-D3/D4, 2026-10-03).
# Starts a game-only window (JGLauncher.exe -game, or a packaged <Name>.exe), captures the client area,
# finds the GameUIDemo orange "End Turn" button by color, clicks it with posted mouse messages (no cursor or
# foreground change), captures again, then closes the window with WM_CLOSE and prints the exit code.
# usage: powershell -NoProfile -ExecutionPolicy Bypass -File gamehost_e2e.ps1 -Exe <exe> [-Arguments "-game"] [-WorkDir <dir>] -OutDir <dir> [-WaitSeconds 12]
# Check the game log afterwards (dev: <Bin>\jg_log.txt, package: <Package>\Saved\Logs\<Name>.log) for
# "GameHost: started", "End Turn clicked (turn 2)" and error lines.
param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [string]$Arguments = "",
    [string]$WorkDir = "",
    [Parameter(Mandatory = $true)][string]$OutDir,
    [int]$WaitSeconds = 12
)

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class GHE2E {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT point);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern bool PostMessage(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] public static extern IntPtr SetThreadDpiAwarenessContext(IntPtr context);
}
"@
# The packaged game (bootstrap) is per-monitor DPI aware. Measure, capture and click in physical pixels too,
# otherwise a scaled display (e.g. 125%) gives logical sizes and a cropped capture.
[GHE2E]::SetThreadDpiAwarenessContext([IntPtr]-4) | Out-Null   # DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
if ($WorkDir -eq "") { $WorkDir = Split-Path -Parent $Exe }

if (-not (Test-Path -LiteralPath $Exe -PathType Leaf)) { Write-Output "No such exe: $Exe"; exit 1 }
$startArgs = @{ FilePath = $Exe; WorkingDirectory = $WorkDir; PassThru = $true }
if ($Arguments -ne "") { $startArgs.ArgumentList = $Arguments }
try { $proc = Start-Process @startArgs -ErrorAction Stop } catch { Write-Output ("Could not start {0}: {1}" -f $Exe, $_.Exception.Message); exit 1 }
$null = $proc.Handle

$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt ($WaitSeconds * 4); $i++) {
    Start-Sleep -Milliseconds 250
    $proc.Refresh()
    if ($proc.HasExited) { break }
    if ($proc.MainWindowHandle -ne [IntPtr]::Zero) { $hwnd = $proc.MainWindowHandle }
}
if ($hwnd -eq [IntPtr]::Zero) { Write-Output "No game window (process exited: $($proc.HasExited))"; if (-not $proc.HasExited) { $proc.Kill() }; exit 1 }
Start-Sleep -Seconds 2

function Capture-Client([string]$path) {
    $win = New-Object GHE2E+RECT; [GHE2E]::GetWindowRect($hwnd, [ref]$win) | Out-Null
    $client = New-Object GHE2E+RECT; [GHE2E]::GetClientRect($hwnd, [ref]$client) | Out-Null
    $origin = New-Object GHE2E+POINT; [GHE2E]::ClientToScreen($hwnd, [ref]$origin) | Out-Null
    $bmp = New-Object System.Drawing.Bitmap ($win.Right - $win.Left), ($win.Bottom - $win.Top)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $g.GetHdc()
    [GHE2E]::PrintWindow($hwnd, $hdc, 2) | Out-Null   # PW_RENDERFULLCONTENT: DirectX content too
    $g.ReleaseHdc($hdc); $g.Dispose()
    $rect = New-Object System.Drawing.Rectangle ($origin.X - $win.Left), ($origin.Y - $win.Top), $client.Right, $client.Bottom
    $clientBmp = $bmp.Clone($rect, $bmp.PixelFormat)
    $bmp.Dispose()
    $clientBmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    return $clientBmp
}

$shot = Capture-Client (Join-Path $OutDir "gamehost_1.png")
Write-Output ("Client {0}x{1}" -f $shot.Width, $shot.Height)

# Orange End Turn button (sRGB display 242,115,26). Sample every 2nd pixel.
$minX = [int]::MaxValue; $minY = [int]::MaxValue; $maxX = -1; $maxY = -1; $count = 0
for ($y = 0; $y -lt $shot.Height; $y += 2) {
    for ($x = 0; $x -lt $shot.Width; $x += 2) {
        $c = $shot.GetPixel($x, $y)
        if ([Math]::Abs($c.R - 242) -le 12 -and [Math]::Abs($c.G - 115) -le 12 -and [Math]::Abs($c.B - 26) -le 12) {
            $count++
            if ($x -lt $minX) { $minX = $x }; if ($y -lt $minY) { $minY = $y }
            if ($x -gt $maxX) { $maxX = $x }; if ($y -gt $maxY) { $maxY = $y }
        }
    }
}
$nonBlack = 0
for ($y = 0; $y -lt $shot.Height; $y += 8) { for ($x = 0; $x -lt $shot.Width; $x += 8) { $c = $shot.GetPixel($x, $y); if (($c.R + $c.G + $c.B) -gt 30) { $nonBlack++ } } }
Write-Output ("Non-black samples {0}, orange samples {1}" -f $nonBlack, $count)
$shot.Dispose()

if ($count -ge 50) {
    $cx = [int](($minX + $maxX) / 2); $cy = [int](($minY + $maxY) / 2)
    Write-Output ("End Turn box ({0},{1})-({2},{3}), click ({4},{5})" -f $minX, $minY, $maxX, $maxY, $cx, $cy)
    $lParam = [IntPtr](($cy -shl 16) -bor ($cx -band 0xFFFF))
    [GHE2E]::PostMessage($hwnd, 0x0200, [IntPtr]::Zero, $lParam) | Out-Null   # WM_MOUSEMOVE
    Start-Sleep -Milliseconds 200
    [GHE2E]::PostMessage($hwnd, 0x0201, [IntPtr]1, $lParam) | Out-Null         # WM_LBUTTONDOWN
    Start-Sleep -Milliseconds 100
    [GHE2E]::PostMessage($hwnd, 0x0202, [IntPtr]::Zero, $lParam) | Out-Null    # WM_LBUTTONUP
    Start-Sleep -Seconds 1
    $after = Capture-Client (Join-Path $OutDir "gamehost_2.png"); $after.Dispose()
}
else {
    Write-Output "End Turn button not found"
}

[GHE2E]::PostMessage($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null   # WM_CLOSE
if (-not $proc.WaitForExit(30000)) { Write-Output "Did not exit in 30 s"; $proc.Kill(); exit 1 }
Write-Output ("ExitCode {0}" -f $proc.ExitCode)
