param(
    # GameUIDemo verification game Bin\DevelopEngine (the folder with JGLauncher.exe). Start without imgui.ini (default layout), like gameui_e2e.ps1.
    [Parameter(Mandatory = $true)][string]$BinDir,
    [Parameter(Mandatory = $true)][string]$OutDir,
    [int]$WaitSeconds = 40
)

# Scene Viewport focus loss check (JGEditor, D-22 (2) worktree, 2026-10-03).
# When the app loses OS focus (or the Scene Viewport window loses ImGui focus) while a button is held, the editor sends
# FocusLost to the game viewport (PGameViewport cancels the game UI press / drag) and does not send the later release.
#   A) press End Turn -> WM_KILLFOCUS -> release -> WM_SETFOCUS : "SceneViewport focus lost (app): press canceled", no "End Turn clicked"
#   B) normal click on End Turn                                   : "End Turn clicked (turn 2)" (input works again)
#   C) press the gold card, move (drag begins) -> WM_KILLFOCUS -> release : "drag end Card1: canceled", "focus lost (app): press canceled"
#   D) drag the card onto the drop zone                           : "dropped on DropZone" (drag works again)
# Messages go only to the launcher window (PostMessage). WM_KILLFOCUS / WM_SETFOCUS are posted, the real focus does not move;
# ImGui's Win32 backend turns them into io.AddFocusEvent (AppFocusLost for one frame), which is the path a real focus change takes.
# Result: the game Bin jg_log.txt lines printed at the end.

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class FocusCheck {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X; public int Y; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT lpPoint);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdcBlt, uint nFlags);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] public static extern IntPtr GetWindowDpiAwarenessContext(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern int GetAwarenessFromDpiAwarenessContext(IntPtr value);
    [DllImport("user32.dll")] public static extern IntPtr SetThreadDpiAwarenessContext(IntPtr dpiContext);
}
"@

function Capture([IntPtr]$hwnd, [string]$path) {
    $rect = New-Object FocusCheck+RECT
    [FocusCheck]::GetWindowRect($hwnd, [ref]$rect) | Out-Null
    $bmp = New-Object System.Drawing.Bitmap(($rect.Right - $rect.Left), ($rect.Bottom - $rect.Top))
    $gfx = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $gfx.GetHdc()
    [FocusCheck]::PrintWindow($hwnd, $hdc, 2) | Out-Null   # PW_RENDERFULLCONTENT
    $gfx.ReleaseHdc($hdc); $gfx.Dispose()
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    return $bmp
}

# Orange End Turn (code color 0.95, 0.45, 0.10 -> (242, 115, 26) on the sRGB display texture). Window coordinates.
function Find-Orange([System.Drawing.Bitmap]$bmp) {
    $rectAll = New-Object System.Drawing.Rectangle(0, 0, $bmp.Width, $bmp.Height)
    $data = $bmp.LockBits($rectAll, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $bytes = New-Object byte[] ($data.Stride * $bmp.Height)
    [System.Runtime.InteropServices.Marshal]::Copy($data.Scan0, $bytes, 0, $bytes.Length)
    $bmp.UnlockBits($data)
    $xs = New-Object System.Collections.Generic.List[int]
    $ys = New-Object System.Collections.Generic.List[int]
    for ($y = 0; $y -lt $bmp.Height; $y++) {
        $row = $y * $data.Stride
        for ($x = 0; $x -lt $bmp.Width; $x++) {
            $i = $row + $x * 4
            if ([Math]::Abs($bytes[$i + 2] - 242) -le 6 -and [Math]::Abs($bytes[$i + 1] - 115) -le 6 -and [Math]::Abs($bytes[$i] - 26) -le 6) { $xs.Add($x); $ys.Add($y) }
        }
    }
    if ($xs.Count -eq 0) { return @{ Count = 0 } }
    $sx = $xs.ToArray(); [Array]::Sort($sx); $sy = $ys.ToArray(); [Array]::Sort($sy)
    $mx = $sx[[int]($sx.Length / 2)]; $my = $sy[[int]($sy.Length / 2)]
    $minX = [int]::MaxValue; $minY = [int]::MaxValue; $maxX = -1; $maxY = -1; $count = 0
    for ($k = 0; $k -lt $xs.Count; $k++) {
        $x = $xs[$k]; $y = $ys[$k]
        if ([Math]::Abs($x - $mx) -le 200 -and [Math]::Abs($y - $my) -le 200) {
            $count++
            if ($x -lt $minX) { $minX = $x }; if ($x -gt $maxX) { $maxX = $x }
            if ($y -lt $minY) { $minY = $y }; if ($y -gt $maxY) { $maxY = $y }
        }
    }
    return @{ Count = $count; MinX = $minX; MinY = $minY; MaxX = $maxX; MaxY = $maxY }
}

function Post-Mouse([IntPtr]$hwnd, [int]$msg, [int]$keys, [int]$clientX, [int]$clientY) {
    $lParam = [IntPtr](($clientY -shl 16) -bor ($clientX -band 0xFFFF))
    [FocusCheck]::PostMessage($hwnd, $msg, [IntPtr]$keys, $lParam) | Out-Null
}

function Post-Focus([IntPtr]$hwnd, [bool]$gained) {
    if ($gained) { [FocusCheck]::PostMessage($hwnd, 0x0007, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null }   # WM_SETFOCUS
    else         { [FocusCheck]::PostMessage($hwnd, 0x0008, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null }   # WM_KILLFOCUS
}

$logPath = Join-Path $BinDir "jg_log.txt"
$exe = Join-Path $BinDir "JGLauncher.exe"
$proc = Start-Process -FilePath $exe -WorkingDirectory $BinDir -PassThru
Write-Output "Started PID $($proc.Id)"
$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 120; $i++) {
    Start-Sleep -Milliseconds 500
    $proc.Refresh()
    if ($proc.HasExited) { Write-Output "Process exited early with code $($proc.ExitCode)"; exit 2 }
    if ($proc.MainWindowHandle -ne [IntPtr]::Zero) { $hwnd = $proc.MainWindowHandle; break }
}
if ($hwnd -eq [IntPtr]::Zero) { Write-Output "No main window"; $proc.Kill(); exit 3 }
$windowAwareness = [FocusCheck]::GetAwarenessFromDpiAwarenessContext([FocusCheck]::GetWindowDpiAwarenessContext($hwnd))
if ($windowAwareness -ne 0) { [FocusCheck]::SetThreadDpiAwarenessContext([IntPtr](-4)) | Out-Null }   # PER_MONITOR_AWARE_V2: physical pixels
Write-Output "Main window $hwnd, DPI awareness $windowAwareness"
Start-Sleep -Seconds $WaitSeconds

$winRect = New-Object FocusCheck+RECT
[FocusCheck]::GetWindowRect($hwnd, [ref]$winRect) | Out-Null
$origin = New-Object FocusCheck+POINT
[FocusCheck]::ClientToScreen($hwnd, [ref]$origin) | Out-Null
$offX = $origin.X - $winRect.Left; $offY = $origin.Y - $winRect.Top

$before = Capture $hwnd (Join-Path $OutDir "focuslost_before.png")
$found = Find-Orange $before
$before.Dispose()
if ($found.Count -lt 300) { Write-Output "End Turn button not found"; [FocusCheck]::PostMessage($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null; $proc.WaitForExit(30000) | Out-Null; exit 4 }
# Viewport image rectangle from the button (End Turn 340 wide, margins 40, reference 1920x1080, 16:9) - same as gameui_e2e.ps1.
$imageW = ($found.MaxX - $found.MinX + 1) * 1920.0 / 340.0; $imageH = $imageW * 720.0 / 1280.0
$imageLeft = $found.MaxX + $imageW * 40.0 / 1920.0 - $imageW
$imageTop  = $found.MaxY + $imageH * 40.0 / 1080.0 - $imageH
function Ref-ToClient([double]$refX, [double]$refY) {
    return @(([int]($imageLeft + $imageW * $refX / 1920.0) - $offX), ([int]($imageTop + $imageH * $refY / 1080.0) - $offY))
}
$endTurn = Ref-ToClient 1710 985
$card    = Ref-ToClient 330 914
$cardMid = Ref-ToClient 560 914
$drop    = Ref-ToClient 1010 914
Write-Output ("End Turn client ({0}, {1}), gold card ({2}, {3}), drop zone ({4}, {5})" -f $endTurn[0], $endTurn[1], $card[0], $card[1], $drop[0], $drop[1])

Write-Output "A: press End Turn -> focus lost -> release"
Post-Mouse $hwnd 0x0200 0 $endTurn[0] $endTurn[1]   # WM_MOUSEMOVE
Post-Mouse $hwnd 0x0201 1 $endTurn[0] $endTurn[1]   # WM_LBUTTONDOWN
Start-Sleep -Milliseconds 600
Post-Focus $hwnd $false
Start-Sleep -Milliseconds 600
Post-Mouse $hwnd 0x0200 0 $endTurn[0] $endTurn[1]
Post-Mouse $hwnd 0x0202 0 $endTurn[0] $endTurn[1]   # WM_LBUTTONUP
Start-Sleep -Milliseconds 800
Post-Focus $hwnd $true
Start-Sleep -Milliseconds 600

Write-Output "B: normal click on End Turn"
Post-Mouse $hwnd 0x0200 0 $endTurn[0] $endTurn[1]
Post-Mouse $hwnd 0x0201 1 $endTurn[0] $endTurn[1]
Post-Mouse $hwnd 0x0202 0 $endTurn[0] $endTurn[1]
Start-Sleep -Milliseconds 800

Write-Output "C: drag the gold card -> focus lost -> release"
Post-Mouse $hwnd 0x0200 0 $card[0] $card[1]
Post-Mouse $hwnd 0x0201 1 $card[0] $card[1]
Start-Sleep -Milliseconds 300
for ($s = 1; $s -le 3; $s++) {
    Post-Mouse $hwnd 0x0200 1 ([int]($card[0] + ($cardMid[0] - $card[0]) * $s / 3)) $card[1]   # WM_MOUSEMOVE with MK_LBUTTON
    Start-Sleep -Milliseconds 120
}
Start-Sleep -Milliseconds 300
Post-Focus $hwnd $false
Start-Sleep -Milliseconds 600
Post-Mouse $hwnd 0x0200 0 $cardMid[0] $cardMid[1]
Post-Mouse $hwnd 0x0202 0 $cardMid[0] $cardMid[1]
Start-Sleep -Milliseconds 800
Post-Focus $hwnd $true
Start-Sleep -Milliseconds 600
$mid = Capture $hwnd (Join-Path $OutDir "focuslost_after_cancel.png")
$mid.Dispose()

Write-Output "D: drag the gold card onto the drop zone"
Post-Mouse $hwnd 0x0200 0 $card[0] $card[1]
Post-Mouse $hwnd 0x0201 1 $card[0] $card[1]
Start-Sleep -Milliseconds 300
for ($s = 1; $s -le 4; $s++) {
    Post-Mouse $hwnd 0x0200 1 ([int]($card[0] + ($drop[0] - $card[0]) * $s / 5)) $card[1]
    Start-Sleep -Milliseconds 100
}
Post-Mouse $hwnd 0x0200 1 $drop[0] $drop[1]
Post-Mouse $hwnd 0x0202 0 $drop[0] $drop[1]
Start-Sleep -Milliseconds 1000
$after = Capture $hwnd (Join-Path $OutDir "focuslost_after.png")
$after.Dispose()

[FocusCheck]::PostMessage($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null   # WM_CLOSE
if (-not $proc.WaitForExit(60000)) { Write-Output "Timeout waiting exit, killing"; $proc.Kill(); exit 5 }
Write-Output "ExitCode $($proc.ExitCode)"
Select-String -Path $logPath -Pattern "SceneViewport Click|focus lost|End Turn clicked|drag begin|drag end|\]\[error\]|\]\[critical\]|ID3D12|live blocks" | ForEach-Object { $_.Line }
exit 0
