param(
    # Bin\DevelopEngine (the folder with JGLauncher.exe)
    [Parameter(Mandatory = $true)][string]$BinDir,
    [Parameter(Mandatory = $true)][string]$OutPrefix,
    [int]$WaitSeconds = 20,
    # Extra launcher arguments, e.g. "-game"
    [string]$LauncherArgs = ""
)

# D-22 (2) check: launch -> report process DPI awareness (GetProcessDpiAwareness: 0 unaware, 1 system, 2 per monitor),
# window / client size in physical pixels, window DPI -> capture (physical) -> WM_CLOSE -> exit code.
# This script runs per-monitor DPI aware so every size it prints is a physical pixel size.

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class EdDpiWin32 {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
    [DllImport("user32.dll")] public static extern IntPtr SetThreadDpiAwarenessContext(IntPtr dpiContext);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern uint GetDpiForWindow(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdcBlt, uint nFlags);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("shcore.dll")] public static extern int GetProcessDpiAwareness(IntPtr hprocess, out int value);
    [DllImport("kernel32.dll")] public static extern IntPtr OpenProcess(uint access, bool inherit, int pid);
    [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr handle);
}
"@

# DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = -4
[EdDpiWin32]::SetThreadDpiAwarenessContext([IntPtr](-4)) | Out-Null

$exe  = Join-Path $BinDir "JGLauncher.exe"
if ($LauncherArgs -ne "") {
    $proc = Start-Process -FilePath $exe -WorkingDirectory $BinDir -ArgumentList $LauncherArgs -PassThru
} else {
    $proc = Start-Process -FilePath $exe -WorkingDirectory $BinDir -PassThru
}
Write-Output "Started PID $($proc.Id) $LauncherArgs"

$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 120; $i++) {
    Start-Sleep -Milliseconds 500
    $proc.Refresh()
    if ($proc.HasExited) { Write-Output "Process exited early with code $($proc.ExitCode)"; exit 2 }
    if ($proc.MainWindowHandle -ne [IntPtr]::Zero) { $hwnd = $proc.MainWindowHandle; break }
}
if ($hwnd -eq [IntPtr]::Zero) { Write-Output "No main window"; $proc.Kill(); exit 3 }
Start-Sleep -Seconds $WaitSeconds

# PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
$handle = [EdDpiWin32]::OpenProcess(0x1000, $false, $proc.Id)
$awareness = -1
[EdDpiWin32]::GetProcessDpiAwareness($handle, [ref]$awareness) | Out-Null
[EdDpiWin32]::CloseHandle($handle) | Out-Null

$win = New-Object EdDpiWin32+RECT
$cli = New-Object EdDpiWin32+RECT
[EdDpiWin32]::GetWindowRect($hwnd, [ref]$win) | Out-Null
[EdDpiWin32]::GetClientRect($hwnd, [ref]$cli) | Out-Null
$dpi = [EdDpiWin32]::GetDpiForWindow($hwnd)
Write-Output ("GetProcessDpiAwareness {0}; window DPI {1} ({2:P0}); window {3}x{4}; client {5}x{6} (physical)" -f `
    $awareness, $dpi, ($dpi / 96.0), ($win.Right - $win.Left), ($win.Bottom - $win.Top), $cli.Right, $cli.Bottom)

$width  = $win.Right - $win.Left
$height = $win.Bottom - $win.Top
$bmp = New-Object System.Drawing.Bitmap($width, $height)
$gfx = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $gfx.GetHdc()
[EdDpiWin32]::PrintWindow($hwnd, $hdc, 2) | Out-Null   # PW_RENDERFULLCONTENT
$gfx.ReleaseHdc($hdc)
$gfx.Dispose()
$bmp.Save("${OutPrefix}.png", [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Output "Saved ${OutPrefix}.png (${width}x${height})"

[EdDpiWin32]::PostMessage($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
if (-not $proc.WaitForExit(60000)) { Write-Output "Timeout waiting exit, killing"; $proc.Kill(); exit 5 }
Write-Output "ExitCode $($proc.ExitCode)"
exit 0
