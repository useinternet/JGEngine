param(
    # Bin\DevelopEngine of a game project (the folder with JGLauncher.exe)
    [Parameter(Mandatory = $true)][string]$BinDir,
    [Parameter(Mandatory = $true)][string]$OutPrefix,
    [int]$WaitSeconds = 20,
    # Shrink the editor window by this many physical pixels (width and height) in $Steps quick steps, then wait.
    [int]$ShrinkX = 600,
    [int]$ShrinkY = 300,
    [int]$Steps = 10,
    [int]$StepMilliseconds = 40,
    [int]$SettleSeconds = 3
)

# D-22 (1) check for the JGEditor Scene Viewport: render size follows the visible image size, once, after the size stops changing.
# Launch -> wait -> capture "a" -> resize the editor window in quick steps (no resize expected while it moves)
# -> wait -> capture "b" -> WM_CLOSE -> exit code. Read the result from Bin\DevelopEngine\jg_log.txt:
#   "SceneViewport render size AxB -> CxD (game UI scale S)" lines (one per settled size).
# Use an imgui.ini that docks the Scene Viewport into the main dock space so it follows the OS window size.

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class SvResizeWin32 {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdcBlt, uint nFlags);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
}
"@

[SvResizeWin32]::SetProcessDPIAware() | Out-Null
$SWP_NOMOVE_NOZORDER_NOACTIVATE = 0x0002 -bor 0x0004 -bor 0x0010

function Save-Capture([IntPtr]$hwnd, [string]$path) {
    $rect = New-Object SvResizeWin32+RECT
    [SvResizeWin32]::GetWindowRect($hwnd, [ref]$rect) | Out-Null
    $width  = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    $bmp = New-Object System.Drawing.Bitmap($width, $height)
    $gfx = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $gfx.GetHdc()
    $ok = [SvResizeWin32]::PrintWindow($hwnd, $hdc, 2)   # PW_RENDERFULLCONTENT
    $gfx.ReleaseHdc($hdc)
    if (-not $ok) { $gfx.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bmp.Size) }
    $gfx.Dispose()
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Output "Saved $path (${width}x${height})"
}

$exe  = Join-Path $BinDir "JGLauncher.exe"
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

Start-Sleep -Seconds $WaitSeconds
Save-Capture $hwnd "${OutPrefix}_a.png"

$rect = New-Object SvResizeWin32+RECT
[SvResizeWin32]::GetWindowRect($hwnd, [ref]$rect) | Out-Null
$width0  = $rect.Right - $rect.Left
$height0 = $rect.Bottom - $rect.Top
Write-Output "Window (physical) ${width0}x${height0}; shrinking by ${ShrinkX}x${ShrinkY} in $Steps steps, $StepMilliseconds ms apart"
for ($s = 1; $s -le $Steps; $s++) {
    $w = [int]($width0 - $ShrinkX * $s / $Steps)
    $h = [int]($height0 - $ShrinkY * $s / $Steps)
    [SvResizeWin32]::SetWindowPos($hwnd, [IntPtr]::Zero, 0, 0, $w, $h, $SWP_NOMOVE_NOZORDER_NOACTIVATE) | Out-Null
    Start-Sleep -Milliseconds $StepMilliseconds
}
Write-Output "Resize steps done at $(Get-Date -Format 'HH:mm:ss.fff')"
Start-Sleep -Seconds $SettleSeconds
Save-Capture $hwnd "${OutPrefix}_b.png"

[SvResizeWin32]::PostMessage($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
if (-not $proc.WaitForExit(60000)) { Write-Output "Timeout waiting exit, killing"; $proc.Kill(); exit 5 }
Write-Output "ExitCode $($proc.ExitCode)"
exit 0
