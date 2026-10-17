param(
    [string]$BinDir,
    [int]$Trials = 20,
    [int]$WaitSeconds = 40,
    [string]$Method = "A"
)
# 입력 빠짐 원인 실험(GG-H32). End Turn 을 세 가지 방법으로 Trials 번씩 누르고, 게임 로그의 "End Turn clicked" 가 늘었는지 센다.
#  A = 지금 e2e 와 같음(PowerShell 에서 이동 · 누름 · 뗌을 따로따로 PostMessage)
#  B = 이동을 보내고 30ms 기다린 뒤 누름 · 뗌(앱이 이동과 그 뒤 WM_MOUSELEAVE 를 먼저 처리하게) — 원인이 맞다면 거의 다 빠진다
#  C = C# 안에서 세 메시지를 잇달아 PostMessage(사이 간격 수 µs)
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
public class Probe {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X; public int Y; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT lpPoint);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdcBlt, uint nFlags);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] public static extern IntPtr GetWindowDpiAwarenessContext(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern int GetAwarenessFromDpiAwarenessContext(IntPtr value);
    [DllImport("user32.dll")] public static extern IntPtr SetThreadDpiAwarenessContext(IntPtr dpiContext);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern bool GetCursorPos(out POINT p);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern IntPtr SendMessageTimeout(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam, uint flags, uint timeout, out IntPtr result);
    // 앱 스레드가 메시지를 처리할 때까지 기다린다(WM_NULL). 돌아온 시각 = 앱이 프레임 앞머리에서 큐를 비우는 중.
    public static double SyncFrame(IntPtr hwnd) {
        IntPtr result;
        Stopwatch sw = Stopwatch.StartNew();
        SendMessageTimeout(hwnd, 0x0000, IntPtr.Zero, IntPtr.Zero, 0x0002, 1000, out result);   // SMTO_ABORTIFHUNG
        return sw.Elapsed.TotalMilliseconds;
    }
    public static void PostClickSynced(IntPtr hwnd, int x, int y, int delayMs) {
        SyncFrame(hwnd);
        System.Threading.Thread.Sleep(delayMs);
        PostClick(hwnd, x, y);
    }
    public static void PostClick(IntPtr hwnd, int x, int y) {
        IntPtr lp = (IntPtr)((y << 16) | (x & 0xFFFF));
        PostMessage(hwnd, 0x0200, IntPtr.Zero, lp);
        PostMessage(hwnd, 0x0201, (IntPtr)1, lp);
        PostMessage(hwnd, 0x0202, IntPtr.Zero, lp);
    }
}
"@

function Read-Log([string]$path) {
    $stream = [System.IO.File]::Open($path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
    $reader = New-Object System.IO.StreamReader($stream)
    $text = $reader.ReadToEnd()
    $reader.Dispose()
    return $text
}

function Count-EndTurn([string]$path) {
    return ([regex]::Matches((Read-Log $path), "End Turn clicked")).Count
}

function Find-Color([System.Drawing.Bitmap]$bmp, [int]$targetR, [int]$targetG, [int]$targetB) {
    $rectAll = New-Object System.Drawing.Rectangle(0, 0, $bmp.Width, $bmp.Height)
    $data = $bmp.LockBits($rectAll, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $bytes = New-Object byte[] ($data.Stride * $bmp.Height)
    [System.Runtime.InteropServices.Marshal]::Copy($data.Scan0, $bytes, 0, $bytes.Length)
    $bmp.UnlockBits($data)
    $minX = [int]::MaxValue; $minY = [int]::MaxValue; $maxX = -1; $maxY = -1; $count = 0
    for ($y = 0; $y -lt $bmp.Height; $y++) {
        $row = $y * $data.Stride
        for ($x = 0; $x -lt $bmp.Width; $x++) {
            $i = $row + $x * 4
            if ([Math]::Abs($bytes[$i + 2] - $targetR) -le 6 -and [Math]::Abs($bytes[$i + 1] - $targetG) -le 6 -and [Math]::Abs($bytes[$i] - $targetB) -le 6) {
                $count++
                if ($x -lt $minX) { $minX = $x }; if ($x -gt $maxX) { $maxX = $x }
                if ($y -lt $minY) { $minY = $y }; if ($y -gt $maxY) { $maxY = $y }
            }
        }
    }
    return @{ Count = $count; MinX = $minX; MinY = $minY; MaxX = $maxX; MaxY = $maxY }
}

$exe = Join-Path $BinDir "JGLauncher.exe"
$logPath = Join-Path $BinDir "jg_log.txt"
$proc = Start-Process -FilePath $exe -WorkingDirectory $BinDir -PassThru
$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 120; $i++) {
    Start-Sleep -Milliseconds 500
    $proc.Refresh()
    if ($proc.MainWindowHandle -ne [IntPtr]::Zero) { $hwnd = $proc.MainWindowHandle; break }
}
if ($hwnd -eq [IntPtr]::Zero) { Write-Output "No main window"; $proc.Kill(); exit 3 }
$awareness = [Probe]::GetAwarenessFromDpiAwarenessContext([Probe]::GetWindowDpiAwarenessContext($hwnd))
if ($awareness -ne 0) { [Probe]::SetThreadDpiAwarenessContext([IntPtr](-4)) | Out-Null }
Start-Sleep -Seconds $WaitSeconds

$winRect = New-Object Probe+RECT
[Probe]::GetWindowRect($hwnd, [ref]$winRect) | Out-Null
$origin = New-Object Probe+POINT
[Probe]::ClientToScreen($hwnd, [ref]$origin) | Out-Null
$offX = $origin.X - $winRect.Left; $offY = $origin.Y - $winRect.Top
$w = $winRect.Right - $winRect.Left; $h = $winRect.Bottom - $winRect.Top
$bmp = New-Object System.Drawing.Bitmap($w, $h)
$gfx = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $gfx.GetHdc()
[Probe]::PrintWindow($hwnd, $hdc, 2) | Out-Null
$gfx.ReleaseHdc($hdc); $gfx.Dispose()
$found = Find-Color $bmp 242 115 26
$bmp.Dispose()
if ($found.Count -lt 300) { Write-Output "End Turn not found"; [Probe]::PostMessage($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null; exit 4 }
$cx = [int](($found.MinX + $found.MaxX) / 2) - $offX
$cy = [int](($found.MinY + $found.MaxY) / 2) - $offY
$lParam = [IntPtr](($cy -shl 16) -bor ($cx -band 0xFFFF))
$cursor = New-Object Probe+POINT
[Probe]::GetCursorPos([ref]$cursor) | Out-Null
Write-Output ("End Turn client ({0}, {1}); foreground is target: {2}; real cursor ({3}, {4}), window ({5},{6})-({7},{8})" -f $cx, $cy, ([Probe]::GetForegroundWindow() -eq $hwnd), $cursor.X, $cursor.Y, $winRect.Left, $winRect.Top, $winRect.Right, $winRect.Bottom)

# PowerShell 한 줄마다 걸리는 시간(지금 e2e 의 메시지 사이 간격)
$sw = [System.Diagnostics.Stopwatch]::StartNew()
for ($k = 0; $k -lt 200; $k++) { [Probe]::PostMessage($hwnd, 0x0000, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null }
$sw.Stop()
Write-Output ("PowerShell PostMessage statement: {0:N3} ms each (WM_NULL x200)" -f ($sw.Elapsed.TotalMilliseconds / 200))
Start-Sleep -Milliseconds 500

# 앱 프레임 간격: WM_NULL 을 잇달아 보내 돌아오는 간격
$intervals = @()
for ($k = 0; $k -lt 30; $k++) { $intervals += [Probe]::SyncFrame($hwnd) }
$sorted = $intervals | Sort-Object
Write-Output ("SendMessage(WM_NULL) wait: min {0:N2} ms, median {1:N2} ms, max {2:N2} ms" -f $sorted[0], $sorted[15], $sorted[29])

foreach ($method in @($Method)) {
    $ok = 0
    for ($t = 0; $t -lt $Trials; $t++) {
        $before = Count-EndTurn $logPath
        if ($method -eq "A") {
            [Probe]::PostMessage($hwnd, 0x0200, [IntPtr]::Zero, $lParam) | Out-Null
            [Probe]::PostMessage($hwnd, 0x0201, [IntPtr]1, $lParam) | Out-Null
            [Probe]::PostMessage($hwnd, 0x0202, [IntPtr]::Zero, $lParam) | Out-Null
        }
        elseif ($method -eq "B") {
            [Probe]::PostMessage($hwnd, 0x0200, [IntPtr]::Zero, $lParam) | Out-Null
            Start-Sleep -Milliseconds 30
            [Probe]::PostMessage($hwnd, 0x0201, [IntPtr]1, $lParam) | Out-Null
            [Probe]::PostMessage($hwnd, 0x0202, [IntPtr]::Zero, $lParam) | Out-Null
        }
        elseif ($method -eq "C") {
            [Probe]::PostClick($hwnd, $cx, $cy)
        }
        else {
            [Probe]::PostClickSynced($hwnd, $cx, $cy, 3)
        }
        Start-Sleep -Milliseconds 700
        if ((Count-EndTurn $logPath) -gt $before) { $ok++ }
    }
    Write-Output ("Method {0}: sent {1} clicks (count from the final log)" -f $method, $Trials)
}

[Probe]::PostMessage($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
if (-not $proc.WaitForExit(30000)) { $proc.Kill() }
Write-Output "ExitCode $($proc.ExitCode)"
Write-Output ("Final log: End Turn clicked {0}, SceneViewport Click {1}" -f ([regex]::Matches((Read-Log $logPath), "End Turn clicked")).Count, ([regex]::Matches((Read-Log $logPath), "SceneViewport Click")).Count)
