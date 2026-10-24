param(
    [Parameter(Mandatory = $true)][string]$ExeA,
    [Parameter(Mandatory = $true)][string]$ExeB,
    [Parameter(Mandatory = $true)][string]$OutPrefix,
    [int]$LoadSeconds = 20,
    # -Probe: capture both windows once and stop (read the control coordinates from the capture)
    [switch]$Probe,
    # Coordinates are window pixels of a PrintWindow capture (title bar included). Read them from a -Probe capture.
    [int]$HostX = 0, [int]$HostY = 0,
    [int]$JoinX = 0, [int]$JoinY = 0,
    [int]$LeaveX = 0, [int]$LeaveY = 0,
    [int]$ReadyX = 0, [int]$ReadyY = 0,
    [int]$LocalX = 0, [int]$LocalY = 0,
    [int]$ValueX = 0, [int]$ValueY = 0,
    [int]$SetX = 0, [int]$SetY = 0,
    [int]$RemoveX = 0, [int]$RemoveY = 0,
    # Port field: both windows switch to -Port first (47771 may be in use by another test on this PC)
    [int]$PortX = 0, [int]$PortY = 0,
    [string]$Port = "47795"
)

# Lobby (room) check of the Session Lobby window — D-38 session data, seats released before the game, ready marks.
# Two engine editors without a project (A, B): GameFrameWorks runs with no world, so the session stays in the lobby (Ready).
#   A: this machine only + Host -> A: value "v1" + Set Data -> B: Join (B reads room = v1)
#   -> B: Ready -> A: Ready (both ready on both) -> A: value "v2" + Set Data (B reads v2)
#   -> B: Leave (A: slot 1 freed) -> B: Join again (late joiner reads v2) -> A: Remove Data (B: 0 keys)
# Input goes to each editor window with PostMessage only (the foreground window and the real cursor are not touched).
# Needs a verification build with the worktree-only ImGui hook (JG_UITEST=1):
#   Tools/Common/uitest_hook/datatable_uitest_hook.patch.txt. Never apply it to the main tree.
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32RoomCheck {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X; public int Y; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdcBlt, uint nFlags);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT lpPoint);
    [DllImport("user32.dll")] public static extern IntPtr SetThreadDpiAwarenessContext(IntPtr context);
}
"@
# The editor is DPI aware (D-22 2). Work in physical pixels so window rects, captures and posted client coordinates agree.
[Win32RoomCheck]::SetThreadDpiAwarenessContext([IntPtr]-4) | Out-Null   # DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2

function Find-LauncherWindow([string]$exe) {
    for ($i = 0; $i -lt 120; $i++) {
        $procs = Get-Process -Name JGLauncher -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $exe }
        foreach ($p in $procs) {
            $p.Refresh()
            if ($p.MainWindowHandle -ne [IntPtr]::Zero) { return $p.MainWindowHandle }
        }
        Start-Sleep -Milliseconds 500
    }
    throw "No launcher window for $exe"
}

function Save-Capture([IntPtr]$hwnd, [string]$path) {
    $rect = New-Object Win32RoomCheck+RECT
    [Win32RoomCheck]::GetWindowRect($hwnd, [ref]$rect) | Out-Null
    $bmp = New-Object System.Drawing.Bitmap(($rect.Right - $rect.Left), ($rect.Bottom - $rect.Top))
    $gfx = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $gfx.GetHdc()
    $ok = [Win32RoomCheck]::PrintWindow($hwnd, $hdc, 2)
    $gfx.ReleaseHdc($hdc)
    $gfx.Dispose()
    if (-not $ok) { Write-Output "PrintWindow failed" }
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Output "Saved $path"
}

function Get-ClientParam([IntPtr]$hwnd, [int]$windowX, [int]$windowY) {
    # window pixel (capture) -> client pixel
    $rect = New-Object Win32RoomCheck+RECT
    [Win32RoomCheck]::GetWindowRect($hwnd, [ref]$rect) | Out-Null
    $origin = New-Object Win32RoomCheck+POINT
    [Win32RoomCheck]::ClientToScreen($hwnd, [ref]$origin) | Out-Null
    $x = $windowX - ($origin.X - $rect.Left)
    $y = $windowY - ($origin.Y - $rect.Top)
    return @{ X = $x; Y = $y; LParam = [IntPtr](($y -shl 16) -bor ($x -band 0xFFFF)) }
}

function Post-Click([IntPtr]$hwnd, [int]$windowX, [int]$windowY, [string]$what) {
    $p = Get-ClientParam $hwnd $windowX $windowY
    [Win32RoomCheck]::PostMessage($hwnd, 0x0200, [IntPtr]::Zero, $p.LParam) | Out-Null   # WM_MOUSEMOVE
    Start-Sleep -Milliseconds 150
    [Win32RoomCheck]::PostMessage($hwnd, 0x0201, [IntPtr]1, $p.LParam) | Out-Null         # WM_LBUTTONDOWN
    Start-Sleep -Milliseconds 150
    [Win32RoomCheck]::PostMessage($hwnd, 0x0202, [IntPtr]::Zero, $p.LParam) | Out-Null    # WM_LBUTTONUP
    Start-Sleep -Milliseconds 600
    Write-Output "Clicked $what at client ($($p.X), $($p.Y))"
}

function Post-Text([IntPtr]$hwnd, [string]$text, [string]$what) {
    foreach ($ch in $text.ToCharArray()) {
        [Win32RoomCheck]::PostMessage($hwnd, 0x0102, [IntPtr][int]$ch, [IntPtr]1) | Out-Null   # WM_CHAR
        Start-Sleep -Milliseconds 60
    }
    Start-Sleep -Milliseconds 300
    Write-Output "Typed '$text' ($what)"
}

function Post-Backspace([IntPtr]$hwnd, [int]$count) {
    for ($i = 0; $i -lt $count; $i++) {
        [Win32RoomCheck]::PostMessage($hwnd, 0x0100, [IntPtr]0x08, [IntPtr]0x000E0001) | Out-Null   # WM_KEYDOWN VK_BACK
        Start-Sleep -Milliseconds 60
        [Win32RoomCheck]::PostMessage($hwnd, 0x0101, [IntPtr]0x08, [IntPtr]0xC00E0001) | Out-Null   # WM_KEYUP
        Start-Sleep -Milliseconds 60
    }
    Start-Sleep -Milliseconds 300
    Write-Output "Backspace x$count"
}

function Shot([IntPtr]$hwnd, [string]$name) {
    Start-Sleep -Milliseconds 700
    Save-Capture $hwnd "$($OutPrefix)_$name.png"
}

$hA = Find-LauncherWindow $ExeA
$hB = Find-LauncherWindow $ExeB
Write-Output "Windows: A=$hA B=$hB"
Start-Sleep -Seconds $LoadSeconds

Shot $hA "A0_start"
Shot $hB "B0_start"
if ($Probe) {
    [Win32RoomCheck]::PostMessage($hB, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null   # WM_CLOSE
    [Win32RoomCheck]::PostMessage($hA, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    exit 0
}

# 0) both windows use -Port
foreach ($h in @($hA, $hB)) {
    Post-Click $h $PortX $PortY "port field"
    Post-Backspace $h 8
    Post-Text $h $Port "port"
}

# 1) A hosts on 127.0.0.1 (no firewall prompt) and sets room = v1 (the key field starts with "room")
Post-Click $hA $LocalX $LocalY "A: this machine only"
Post-Click $hA $HostX $HostY "A: Host"
Post-Click $hA $ValueX $ValueY "A: value field"
Post-Text $hA "v1" "A: value"
Post-Click $hA $SetX $SetY "A: Set Data"
Shot $hA "A1_hosted_data"

# 2) B joins (default address 127.0.0.1 and port 47771): Welcome brings room = v1
Post-Click $hB $JoinX $JoinY "B: Join"
Start-Sleep -Seconds 3
Shot $hA "A2_joined"
Shot $hB "B2_joined"

# 3) Ready on both sides
Post-Click $hB $ReadyX $ReadyY "B: Ready"
Start-Sleep -Seconds 1
Post-Click $hA $ReadyX $ReadyY "A: Ready"
Start-Sleep -Seconds 2
Shot $hA "A3_ready"
Shot $hB "B3_ready"

# 4) A changes room to v2: B reads the change
Post-Click $hA $ValueX $ValueY "A: value field"
Post-Backspace $hA 4
Post-Text $hA "v2" "A: value"
Post-Click $hA $SetX $SetY "A: Set Data"
Start-Sleep -Seconds 2
Shot $hB "B4_changed"

# 5) B leaves before the game: A's slot 1 is freed (name -, not connected, not ready)
Post-Click $hB $LeaveX $LeaveY "B: Leave"
Start-Sleep -Seconds 2
Shot $hA "A5_left"
Shot $hB "B5_left"

# 6) B joins again: gets the free seat and the current table (room = v2)
Post-Click $hB $JoinX $JoinY "B: Join again"
Start-Sleep -Seconds 3
Shot $hA "A6_rejoined"
Shot $hB "B6_rejoined"

# 7) A removes room: B shows 0 keys
Post-Click $hA $RemoveX $RemoveY "A: Remove Data"
Start-Sleep -Seconds 2
Shot $hB "B7_removed"

[Win32RoomCheck]::PostMessage($hB, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null   # WM_CLOSE
[Win32RoomCheck]::PostMessage($hA, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
Write-Output "WM_CLOSE posted to both"
exit 0
