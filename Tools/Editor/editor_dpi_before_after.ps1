param(
    # Bin\DevelopEngine (the folder with JGLauncher.exe). Put the same imgui.ini there before each run for the same layout.
    [Parameter(Mandatory = $true)][string]$BinDir,
    # Writes ${OutPrefix}_125.png (primary monitor) and ${OutPrefix}_150.png (secondary monitor).
    [Parameter(Mandatory = $true)][string]$OutPrefix,
    # Main tree Bin: run through <repo>\Build\BatchFiles\MainTree.cmd -name=<this> exec JGLauncher.exe (repo guidelines section 3).
    [string]$MainTreeName = "",
    [string]$LauncherArgs = "",
    [int]$WaitSeconds = 25,
    [int]$MoveWaitSeconds = 8,
    # Window size in physical pixels on both monitors (the editor default at 125% is 2420x1399 aware or not).
    [int]$Width = 2420,
    [int]$Height = 1399,
    [int]$PrimaryX = 200,
    [int]$PrimaryY = 120,
    # Top-left on the secondary monitor. This PC: primary 3840x2160 @125%, secondary 2560x1600 @150% at (-2560,572)-(0,2172).
    [int]$SecondaryX = -2500,
    [int]$SecondaryY = 640
)

# D-22 (2) before / after captures (JGEditor, 2026-10-03). Same window size, same layout, two monitors:
#   launch -> place the window on the primary monitor -> screen capture -> move it to the secondary monitor
#   -> set the same size again (a DPI aware editor resizes itself to the WM_DPICHANGED suggestion first) -> screen capture -> WM_CLOSE.
# Screen capture (CopyFromScreen), not PrintWindow: PrintWindow draws a DPI unaware window at its logical size,
# the screen shows what the user sees (the stretched, blurred bitmap of an unaware window).
# The window is topmost only while it is captured. No input is sent. The script thread is per monitor DPI aware: every size is physical.

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class DpiShot {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
    [DllImport("user32.dll")] public static extern IntPtr SetThreadDpiAwarenessContext(IntPtr dpiContext);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern uint GetDpiForWindow(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] public static extern IntPtr GetWindowDpiAwarenessContext(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern int GetAwarenessFromDpiAwarenessContext(IntPtr value);
    // DWMWA_EXTENDED_FRAME_BOUNDS = 9: the visible frame. GetWindowRect also covers the invisible resize border, which shows what is behind.
    [DllImport("dwmapi.dll")] public static extern int DwmGetWindowAttribute(IntPtr hwnd, int attribute, out RECT value, int size);
}
"@
[DpiShot]::SetThreadDpiAwarenessContext([IntPtr](-4)) | Out-Null   # PER_MONITOR_AWARE_V2

$SWP_NOACTIVATE = 0x0010; $SWP_SHOWWINDOW = 0x0040; $SWP_NOMOVE = 0x0002; $SWP_NOSIZE = 0x0001
$HWND_TOPMOST = [IntPtr](-1); $HWND_NOTOPMOST = [IntPtr](-2)

function Describe([IntPtr]$hwnd) {
    $win = New-Object DpiShot+RECT; $cli = New-Object DpiShot+RECT
    [DpiShot]::GetWindowRect($hwnd, [ref]$win) | Out-Null
    [DpiShot]::GetClientRect($hwnd, [ref]$cli) | Out-Null
    $dpi = [DpiShot]::GetDpiForWindow($hwnd)
    return ("window ({0},{1}) {2}x{3}, client {4}x{5}, window DPI {6} ({7:P0})" -f $win.Left, $win.Top, ($win.Right - $win.Left), ($win.Bottom - $win.Top), $cli.Right, $cli.Bottom, $dpi, ($dpi / 96.0))
}

function Shot([IntPtr]$hwnd, [string]$path) {
    [DpiShot]::SetWindowPos($hwnd, $HWND_TOPMOST, 0, 0, 0, 0, ($SWP_NOMOVE -bor $SWP_NOSIZE -bor $SWP_NOACTIVATE)) | Out-Null
    Start-Sleep -Milliseconds 700
    $win = New-Object DpiShot+RECT
    if ([DpiShot]::DwmGetWindowAttribute($hwnd, 9, [ref]$win, 16) -ne 0) { [DpiShot]::GetWindowRect($hwnd, [ref]$win) | Out-Null }
    $w = $win.Right - $win.Left; $h = $win.Bottom - $win.Top
    $bmp = New-Object System.Drawing.Bitmap($w, $h)
    $gfx = [System.Drawing.Graphics]::FromImage($bmp)
    $gfx.CopyFromScreen($win.Left, $win.Top, 0, 0, (New-Object System.Drawing.Size($w, $h)))
    $gfx.Dispose()
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    [DpiShot]::SetWindowPos($hwnd, $HWND_NOTOPMOST, 0, 0, 0, 0, ($SWP_NOMOVE -bor $SWP_NOSIZE -bor $SWP_NOACTIVATE)) | Out-Null
    Write-Output "Saved $path (${w}x${h})"
}

$exe = Join-Path $BinDir "JGLauncher.exe"
$started = Get-Date
$wrapper = $null
if ($MainTreeName -ne "") {
    $repo = [IO.Path]::GetFullPath((Join-Path $BinDir "..\.."))
    $guard = Join-Path $repo "Build\BatchFiles\MainTree.cmd"
    $wrapper = Start-Process -FilePath $env:ComSpec -ArgumentList ("/d /c {0} -name={1} exec JGLauncher.exe {2}" -f $guard, $MainTreeName, $LauncherArgs) -WorkingDirectory $BinDir -WindowStyle Hidden -PassThru
    Write-Output "Started through MainTree.cmd -name=$MainTreeName (wrapper PID $($wrapper.Id))"
}

$proc = $null
if ($null -ne $wrapper) {
    # MainTree.cmd waits until the tree is free (default up to 6 minutes), then starts the launcher.
    for ($i = 0; $i -lt 900 -and $null -eq $proc; $i++) {
        Start-Sleep -Milliseconds 500
        $proc = Get-Process JGLauncher -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $exe -and $_.StartTime -ge $started.AddSeconds(-1) } | Select-Object -First 1
        if ($null -eq $proc -and $wrapper.HasExited) { Write-Output "MainTree.cmd ended before the launcher started (exit $($wrapper.ExitCode))"; exit 6 }
    }
} elseif ($LauncherArgs -ne "") {
    $proc = Start-Process -FilePath $exe -WorkingDirectory $BinDir -ArgumentList $LauncherArgs -PassThru
} else {
    $proc = Start-Process -FilePath $exe -WorkingDirectory $BinDir -PassThru
}
if ($null -eq $proc) { Write-Output "Launcher process not found"; exit 2 }
Write-Output "Launcher PID $($proc.Id) $exe $LauncherArgs"

$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 120; $i++) {
    Start-Sleep -Milliseconds 500
    $proc.Refresh()
    if ($proc.HasExited) { Write-Output "Process exited early with code $($proc.ExitCode)"; exit 3 }
    if ($proc.MainWindowHandle -ne [IntPtr]::Zero) { $hwnd = $proc.MainWindowHandle; break }
}
if ($hwnd -eq [IntPtr]::Zero) { Write-Output "No main window"; $proc.Kill(); exit 4 }
Start-Sleep -Seconds $WaitSeconds
$awareness = [DpiShot]::GetAwarenessFromDpiAwarenessContext([DpiShot]::GetWindowDpiAwarenessContext($hwnd))
Write-Output ("Window DPI awareness {0} (0 unaware, 2 per monitor); start: {1}" -f $awareness, (Describe $hwnd))

# Primary monitor
[DpiShot]::SetWindowPos($hwnd, [IntPtr]::Zero, $PrimaryX, $PrimaryY, $Width, $Height, ($SWP_NOACTIVATE -bor $SWP_SHOWWINDOW)) | Out-Null
Start-Sleep -Seconds 3
Write-Output ("Primary: " + (Describe $hwnd))
Shot $hwnd "${OutPrefix}_125.png"

# Secondary monitor: move, then set the size again after the DPI change settles
[DpiShot]::SetWindowPos($hwnd, [IntPtr]::Zero, $SecondaryX, $SecondaryY, $Width, $Height, ($SWP_NOACTIVATE -bor $SWP_SHOWWINDOW)) | Out-Null
Start-Sleep -Seconds 2
Write-Output ("Moved: " + (Describe $hwnd))
[DpiShot]::SetWindowPos($hwnd, [IntPtr]::Zero, $SecondaryX, $SecondaryY, $Width, $Height, ($SWP_NOACTIVATE -bor $SWP_SHOWWINDOW)) | Out-Null
Start-Sleep -Seconds $MoveWaitSeconds
Write-Output ("Secondary: " + (Describe $hwnd))
Shot $hwnd "${OutPrefix}_150.png"

[DpiShot]::PostMessage($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null   # WM_CLOSE
if (-not $proc.WaitForExit(60000)) { Write-Output "Timeout waiting exit, killing"; $proc.Kill(); exit 5 }
Write-Output "ExitCode $($proc.ExitCode)"
if ($null -ne $wrapper) {
    if ($wrapper.WaitForExit(30000)) { Write-Output "MainTree.cmd exit $($wrapper.ExitCode)" } else { Write-Output "MainTree.cmd still running" }
}
exit 0
