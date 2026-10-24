param(
    [Parameter(Mandatory = $true)][string]$ExeA,
    [Parameter(Mandatory = $true)][string]$ExeB,
    [Parameter(Mandatory = $true)][string]$OutPrefix,
    [int]$LoadSeconds = 25,
    [int]$MatchSeconds = 15,
    # -Probe: capture both windows once and stop (read the button coordinates from the capture)
    [switch]$Probe,
    # Coordinates are window pixels of a PrintWindow capture (title bar included). Read them from a -Probe capture.
    [int]$HostX = 0, [int]$HostY = 0,
    [int]$JoinX = 0, [int]$JoinY = 0,
    [int]$LeaveX = 0, [int]$LeaveY = 0,
    [int]$ReloadX = 0, [int]$ReloadY = 0,
    [int]$LocalX = 0, [int]$LocalY = 0
)

# Server 5-5 Session Lobby window check: two launchers (A, B) of the same game project, driven only through the lobby window.
#   A: "Host on this machine only" + Host  ->  B: Join  ->  A: Reload World (new match with the current players)  ->  B: Leave
# Input goes to each launcher window with PostMessage only (the foreground window and the real cursor are not touched).
# Needs a verification build with the worktree-only ImGui hook (JG_UITEST=1 makes ImGui use the posted mouse messages):
#   Tools/Common/uitest_hook/datatable_uitest_hook.patch.txt. Never apply it to the main tree.
# Launch the two launchers first (run_net_lobby.sh starts them under crashwalk for logs and exit codes), then run this script.
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32LobbyCheck {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X; public int Y; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdcBlt, uint nFlags);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT lpPoint);
}
"@

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
    $rect = New-Object Win32LobbyCheck+RECT
    [Win32LobbyCheck]::GetWindowRect($hwnd, [ref]$rect) | Out-Null
    $bmp = New-Object System.Drawing.Bitmap(($rect.Right - $rect.Left), ($rect.Bottom - $rect.Top))
    $gfx = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $gfx.GetHdc()
    $ok = [Win32LobbyCheck]::PrintWindow($hwnd, $hdc, 2)
    $gfx.ReleaseHdc($hdc)
    $gfx.Dispose()
    if (-not $ok) { Write-Output "PrintWindow failed" }
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Output "Saved $path"
}

function Post-Click([IntPtr]$hwnd, [int]$windowX, [int]$windowY, [string]$what) {
    # window pixel (capture) -> client pixel
    $rect = New-Object Win32LobbyCheck+RECT
    [Win32LobbyCheck]::GetWindowRect($hwnd, [ref]$rect) | Out-Null
    $origin = New-Object Win32LobbyCheck+POINT
    [Win32LobbyCheck]::ClientToScreen($hwnd, [ref]$origin) | Out-Null
    $x = $windowX - ($origin.X - $rect.Left)
    $y = $windowY - ($origin.Y - $rect.Top)
    $lParam = [IntPtr](($y -shl 16) -bor ($x -band 0xFFFF))
    [Win32LobbyCheck]::PostMessage($hwnd, 0x0200, [IntPtr]::Zero, $lParam) | Out-Null   # WM_MOUSEMOVE
    Start-Sleep -Milliseconds 150
    [Win32LobbyCheck]::PostMessage($hwnd, 0x0201, [IntPtr]1, $lParam) | Out-Null         # WM_LBUTTONDOWN
    Start-Sleep -Milliseconds 150
    [Win32LobbyCheck]::PostMessage($hwnd, 0x0202, [IntPtr]::Zero, $lParam) | Out-Null    # WM_LBUTTONUP
    Start-Sleep -Milliseconds 600
    Write-Output "Clicked $what at client ($x, $y)"
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
    [Win32LobbyCheck]::PostMessage($hB, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null   # WM_CLOSE
    [Win32LobbyCheck]::PostMessage($hA, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    exit 0
}

# 1) A hosts on 127.0.0.1 (no firewall prompt)
Post-Click $hA $LocalX $LocalY "A: this machine only"
Post-Click $hA $HostX $HostY "A: Host"
Shot $hA "A1_hosted"

# 2) B joins (default address 127.0.0.1 and port 47771 in the window)
Post-Click $hB $JoinX $JoinY "B: Join"
Start-Sleep -Seconds 3
Shot $hA "A2_joined"
Shot $hB "B2_joined"

# 3) A starts a new match with the current players
Post-Click $hA $ReloadX $ReloadY "A: Reload World"
Start-Sleep -Seconds $MatchSeconds
Shot $hA "A3_match"
Shot $hB "B3_match"

# 4) B leaves (back to standalone; A keeps the slot as disconnected)
Post-Click $hB $LeaveX $LeaveY "B: Leave"
Start-Sleep -Seconds 2
Shot $hA "A4_left"
Shot $hB "B4_left"

[Win32LobbyCheck]::PostMessage($hB, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null   # WM_CLOSE
[Win32LobbyCheck]::PostMessage($hA, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
Write-Output "WM_CLOSE posted to both"
exit 0
