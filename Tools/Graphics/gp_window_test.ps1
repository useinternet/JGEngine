param(
    [string]$Exe = "",
    [string]$WorkDir = "",
    [int]$AttachPid = 0,
    [int]$StartupWaitSeconds = 12,
    [switch]$AltEnter,
    [string]$CapturePath = ""
)
# GP-G1 / G3 window check (Graphics, 2026-10-02). Starts the launcher, resizes the main window 3 times,
# minimizes / restores it, optionally sends Alt+Enter, captures the window, then closes it with WM_CLOSE.
# Prints client sizes and the exit code. Check D3D12 / error lines in Bin/<Config>/jg_log.txt afterwards.
# usage: powershell -NoProfile -ExecutionPolicy Bypass -File gp_window_test.ps1 -Exe <...\JGLauncher.exe> -WorkDir <...\Bin\DevelopEngine> [-AltEnter] [-CapturePath x.png]
#        -AttachPid <pid> instead of -Exe/-WorkDir drives a process that is already running (e.g. started under dbgevents.exe to see DXGI output).
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class GPWin {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr hWnd, IntPtr after, int x, int y, int cx, int cy, uint flags);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int cmd);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT r);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT r);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint msg, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);
    [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
}
"@

function Get-ClientSizeText([IntPtr]$h) {
    $r = New-Object GPWin+RECT
    [GPWin]::GetClientRect($h, [ref]$r) | Out-Null
    return "$($r.Right - $r.Left)x$($r.Bottom - $r.Top)"
}
function Get-WindowRectText([IntPtr]$h) {
    $r = New-Object GPWin+RECT
    [GPWin]::GetWindowRect($h, [ref]$r) | Out-Null
    return "$($r.Left),$($r.Top) $($r.Right - $r.Left)x$($r.Bottom - $r.Top)"
}

if ($AttachPid -ne 0) {
    $proc = Get-Process -Id $AttachPid
    $null = $proc.Handle   # keep a handle so ExitCode can be read after the process exits
    Write-Output "Attached PID $($proc.Id)"
} else {
    $proc = Start-Process -FilePath $Exe -WorkingDirectory $WorkDir -PassThru
    Write-Output "Started PID $($proc.Id)"
}
$h = [IntPtr]::Zero
for ($i = 0; $i -lt 60; $i++) {
    Start-Sleep -Milliseconds 500
    $proc.Refresh()
    if ($proc.HasExited) { Write-Output "Process exited early with code $($proc.ExitCode)"; exit 2 }
    if ($proc.MainWindowHandle -ne [IntPtr]::Zero) { $h = $proc.MainWindowHandle; break }
}
if ($h -eq [IntPtr]::Zero) { Write-Output "No main window"; $proc.Kill(); exit 3 }
Start-Sleep -Seconds $StartupWaitSeconds
Write-Output "Start client $(Get-ClientSizeText $h)"

# 1) resize 3 times (SWP_NOZORDER | SWP_NOACTIVATE)
$sizes = @(@(1280, 800), @(1600, 900), @(1000, 700))
foreach ($s in $sizes) {
    [GPWin]::SetWindowPos($h, [IntPtr]::Zero, 40, 40, $s[0], $s[1], 0x0014) | Out-Null
    Start-Sleep -Milliseconds 1500
    Write-Output "Resized window to $($s[0])x$($s[1]) -> client $(Get-ClientSizeText $h)"
}

# 2) minimize / restore
[GPWin]::ShowWindow($h, 6) | Out-Null   # SW_MINIMIZE
Start-Sleep -Seconds 2
Write-Output "Minimized -> client $(Get-ClientSizeText $h)"
[GPWin]::ShowWindow($h, 9) | Out-Null   # SW_RESTORE
Start-Sleep -Seconds 2
Write-Output "Restored -> client $(Get-ClientSizeText $h)"

# 3) Alt+Enter (must NOT switch to exclusive fullscreen: window rect stays the same)
if ($AltEnter) {
    $before = Get-WindowRectText $h
    [GPWin]::SetForegroundWindow($h) | Out-Null
    Start-Sleep -Milliseconds 500
    [GPWin]::keybd_event(0x12, 0, 0, [UIntPtr]::Zero)   # VK_MENU down
    [GPWin]::keybd_event(0x0D, 0, 0, [UIntPtr]::Zero)   # VK_RETURN down
    [GPWin]::keybd_event(0x0D, 0, 2, [UIntPtr]::Zero)   # VK_RETURN up
    [GPWin]::keybd_event(0x12, 0, 2, [UIntPtr]::Zero)   # VK_MENU up
    Start-Sleep -Seconds 2
    $after = Get-WindowRectText $h
    Write-Output "Alt+Enter window rect before [$before] after [$after] -> $(if ($before -eq $after) { 'unchanged' } else { 'CHANGED' })"
}

# 4) capture (optional)
if ($CapturePath -ne "") {
    $r = New-Object GPWin+RECT
    [GPWin]::GetWindowRect($h, [ref]$r) | Out-Null
    $bmp = New-Object System.Drawing.Bitmap(($r.Right - $r.Left), ($r.Bottom - $r.Top))
    $gfx = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $gfx.GetHdc()
    [GPWin]::PrintWindow($h, $hdc, 2) | Out-Null
    $gfx.ReleaseHdc($hdc)
    $gfx.Dispose()
    $bmp.Save($CapturePath, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Output "Saved $CapturePath"
}

# 5) close
[GPWin]::PostMessage($h, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
if (-not $proc.WaitForExit(30000)) { Write-Output "Timeout waiting exit, killing"; $proc.Kill(); exit 5 }
Write-Output "ExitCode $($proc.ExitCode)"
exit 0
