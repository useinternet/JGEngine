param(
    # 게임 프로젝트의 Bin\DevelopEngine (JGLauncher.exe 가 있는 폴더)
    [Parameter(Mandatory = $true)][string]$BinDir,
    [Parameter(Mandatory = $true)][string]$OutPrefix,
    [int]$WaitSeconds = 25,
    # 클릭할 점들 "x,y;x,y". 에디터 창 클라이언트 영역 기준 앱 좌표(= ImGui 좌표, DPI 비인식 앱이면 논리 픽셀). 비우면 클릭하지 않는다.
    [string]$Clicks = "",
    # 클릭들 뒤에 오른쪽 버튼으로 끈다 "x1,y1,x2,y2" (씬 뷰포트 궤도 회전). 비우면 하지 않는다.
    [string]$RightDrag = ""
)

# 씬 뷰포트(JGEditor JGSceneViewport) 검증: 게임 프로젝트 런처를 띄우고 → 캡처 → 클릭(실제 마우스 입력) → 캡처 → WM_CLOSE → 종료 코드.
# 클릭 결과는 런처 로그(Bin\DevelopEngine\jg_log.txt)의 "<Controller> click: ..." 줄로 확인한다.
# 포커스가 없는 앱의 첫 클릭은 포커스만 옮기고 메뉴를 열지 않는다. 메뉴를 누를 때는 빈 곳을 먼저 한 번 누른다.
#
# 클릭은 다른 창에 떨어지지 않도록 그 몇 초 동안만 에디터 창을 맨 위(topmost)로 올리고, 끝나면 되돌린다. 커서 위치도 되돌린다.
# 좌표는 앱의 좌표 공간에서 다룬다: 스레드를 DPI 비인식으로 두면 ClientToScreen · SetCursorPos 가 앱과 같은 논리 좌표를 쓴다
# (에디터는 DPI 비인식이라 125% 화면에서 물리 2400x1350 클라이언트가 앱에는 1920x1080 이다).
# ImGui 창이 본 창 밖으로 걸치면 ImGui 가 그 창을 따로 OS 창(같은 프로세스, 본 창 소유)으로 띄운다(150% 화면 1711x1048 에서 DevView 등).
# 그래서 클릭은 그 점의 창이 에디터 프로세스 것이면 누르고, 캡처는 에디터 프로세스의 보이는 창을 모두 앱 좌표로 모아 한 장에 그린다.

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class GfwVerifyWin32 {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X; public int Y; }
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern IntPtr SetThreadDpiAwarenessContext(IntPtr dpiContext);
    [DllImport("user32.dll")] public static extern IntPtr GetWindowDpiAwarenessContext(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern int GetAwarenessFromDpiAwarenessContext(IntPtr value);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT lpPoint);
    [DllImport("user32.dll")] public static extern bool GetCursorPos(out POINT lpPoint);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int X, int Y);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);
    [DllImport("user32.dll")] public static extern IntPtr WindowFromPoint(POINT point);
    [DllImport("user32.dll")] public static extern IntPtr GetAncestor(IntPtr hwnd, uint gaFlags);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdcBlt, uint nFlags);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] public static extern void mouse_event(uint dwFlags, uint dx, uint dy, uint dwData, UIntPtr dwExtraInfo);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, System.Text.StringBuilder lpClassName, int nMaxCount);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetWindowText(IntPtr hWnd, System.Text.StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint processId);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);

    public static uint GetProcessIdOf(IntPtr hWnd) {
        uint processId;
        GetWindowThreadProcessId(hWnd, out processId);
        return processId;
    }

    // 그 프로세스의 보이는 최상위 창들 (EnumWindows 는 위에서 아래 z 순서로 돈다)
    public static IntPtr[] GetVisibleWindowsOf(uint processId) {
        var windows = new System.Collections.Generic.List<IntPtr>();
        EnumWindows((hWnd, lParam) => {
            if (IsWindowVisible(hWnd) && GetProcessIdOf(hWnd) == processId) {
                windows.Add(hWnd);
            }
            return true;
        }, IntPtr.Zero);
        return windows.ToArray();
    }
}
"@

# 캡처는 물리 크기로 받는다.
[GfwVerifyWin32]::SetProcessDPIAware() | Out-Null

$HWND_TOPMOST   = [IntPtr](-1)
$HWND_NOTOPMOST = [IntPtr](-2)
$SWP_NOSIZE_NOMOVE_SHOW = 0x0001 -bor 0x0002 -bor 0x0040
$DPI_UNAWARE    = [IntPtr](-1)
$GA_ROOT        = 2

# 에디터 프로세스의 보이는 창(본 창 + ImGui 가 따로 띄운 창)을 앱 좌표(논리)로 모아 한 장에 그린다.
# 창마다 PrintWindow 로 그리므로 다른 프로그램 화면은 담기지 않는다(창 사이 빈 곳은 검은색).
function Save-WindowCapture([IntPtr]$hwnd, [int]$processId, [string]$path) {
    $previous = [GfwVerifyWin32]::SetThreadDpiAwarenessContext($DPI_UNAWARE)
    try {
        # 아래 창부터 그려 위 창이 덮게 한다 (EnumWindows 는 위 → 아래)
        $windows = @([GfwVerifyWin32]::GetVisibleWindowsOf([uint32]$processId))
        [array]::Reverse($windows)
        if ($windows.Count -eq 0) {
            $windows = @($hwnd)
        }

        $rects = @()
        foreach ($window in $windows) {
            $rect = New-Object GfwVerifyWin32+RECT
            [GfwVerifyWin32]::GetWindowRect($window, [ref]$rect) | Out-Null
            $rects += $rect
        }
        $left   = ($rects | ForEach-Object { $_.Left })   | Measure-Object -Minimum | Select-Object -ExpandProperty Minimum
        $top    = ($rects | ForEach-Object { $_.Top })    | Measure-Object -Minimum | Select-Object -ExpandProperty Minimum
        $right  = ($rects | ForEach-Object { $_.Right })  | Measure-Object -Maximum | Select-Object -ExpandProperty Maximum
        $bottom = ($rects | ForEach-Object { $_.Bottom }) | Measure-Object -Maximum | Select-Object -ExpandProperty Maximum
        $width  = [int]($right - $left)
        $height = [int]($bottom - $top)

        $bmp = New-Object System.Drawing.Bitmap($width, $height)
        $gfx = [System.Drawing.Graphics]::FromImage($bmp)
        $gfx.Clear([System.Drawing.Color]::Black)
        for ($i = 0; $i -lt $windows.Count; $i++) {
            $rect = $rects[$i]
            $windowWidth  = $rect.Right - $rect.Left
            $windowHeight = $rect.Bottom - $rect.Top
            if ($windowWidth -le 0 -or $windowHeight -le 0) {
                continue
            }
            $windowBmp = New-Object System.Drawing.Bitmap($windowWidth, $windowHeight)
            $windowGfx = [System.Drawing.Graphics]::FromImage($windowBmp)
            $hdc = $windowGfx.GetHdc()
            # PW_RENDERFULLCONTENT = 2 : DirectX 스왑체인 창도 캡처된다
            [GfwVerifyWin32]::PrintWindow($windows[$i], $hdc, 2) | Out-Null
            $windowGfx.ReleaseHdc($hdc)
            $windowGfx.Dispose()
            $gfx.DrawImageUnscaled($windowBmp, [int]($rect.Left - $left), [int]($rect.Top - $top))
            $windowBmp.Dispose()
        }
        $gfx.Dispose()
        $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
        $bmp.Dispose()
        Write-Output "Saved $path (${width}x${height}, windows $($windows.Count))"
    }
    finally {
        [GfwVerifyWin32]::SetThreadDpiAwarenessContext($previous) | Out-Null
    }
}

# 그 점의 최상위 창이 에디터 프로세스 것인지 (본 창이 아니어도 된다: ImGui 가 따로 띄운 창)
function Test-EditorWindowAt([GfwVerifyWin32+POINT]$point, [int]$processId) {
    $under = [GfwVerifyWin32]::GetAncestor([GfwVerifyWin32]::WindowFromPoint($point), $GA_ROOT)
    if ($under -eq [IntPtr]::Zero) {
        return $false
    }
    return ([GfwVerifyWin32]::GetProcessIdOf($under) -eq [uint32]$processId)
}

# 건너뛴 클릭의 진단: 그 점의 최상위 창이 누구 것인지
function Get-WindowDescriptionAt([GfwVerifyWin32+POINT]$point) {
    $under = [GfwVerifyWin32]::GetAncestor([GfwVerifyWin32]::WindowFromPoint($point), $GA_ROOT)
    if ($under -eq [IntPtr]::Zero) {
        return "no window"
    }
    $className = New-Object System.Text.StringBuilder 256
    $title     = New-Object System.Text.StringBuilder 256
    [GfwVerifyWin32]::GetClassName($under, $className, 256) | Out-Null
    [GfwVerifyWin32]::GetWindowText($under, $title, 256) | Out-Null
    $ownerId   = [GfwVerifyWin32]::GetProcessIdOf($under)
    $owner     = Get-Process -Id $ownerId -ErrorAction SilentlyContinue
    $ownerName = if ($owner -ne $null) { $owner.ProcessName } else { "?" }
    return "window $under class '$className' title '$title' process $ownerName ($ownerId)"
}

function Invoke-ClientClick([IntPtr]$hwnd, [int]$processId, [int]$x, [int]$y) {
    # 이 스레드만 DPI 비인식으로: 앱 좌표(논리) 그대로 화면 좌표를 구하고 커서를 옮긴다.
    $previous = [GfwVerifyWin32]::SetThreadDpiAwarenessContext($DPI_UNAWARE)
    try {
        $point = New-Object GfwVerifyWin32+POINT
        $point.X = $x
        $point.Y = $y
        [GfwVerifyWin32]::ClientToScreen($hwnd, [ref]$point) | Out-Null

        # 그 점에 실제로 에디터(프로세스)의 창이 있는지 확인한다. 아니면 누르지 않는다.
        if ((Test-EditorWindowAt $point $processId) -eq $false) {
            Write-Output "Skip click ($x, $y): no editor window under the cursor point ($(Get-WindowDescriptionAt $point))"
            return
        }

        # MOUSEEVENTF_LEFTDOWN 0x02, LEFTUP 0x04
        [GfwVerifyWin32]::SetCursorPos($point.X, $point.Y) | Out-Null
        Start-Sleep -Milliseconds 300
        [GfwVerifyWin32]::mouse_event(0x02, 0, 0, 0, [UIntPtr]::Zero)
        Start-Sleep -Milliseconds 150
        [GfwVerifyWin32]::mouse_event(0x04, 0, 0, 0, [UIntPtr]::Zero)
        Write-Output "Clicked client ($x, $y) = logical screen ($($point.X), $($point.Y))"
    }
    finally {
        [GfwVerifyWin32]::SetThreadDpiAwarenessContext($previous) | Out-Null
    }
}

function Invoke-ClientRightDrag([IntPtr]$hwnd, [int]$processId, [int]$x1, [int]$y1, [int]$x2, [int]$y2) {
    $previous = [GfwVerifyWin32]::SetThreadDpiAwarenessContext($DPI_UNAWARE)
    try {
        $from = New-Object GfwVerifyWin32+POINT
        $from.X = $x1
        $from.Y = $y1
        [GfwVerifyWin32]::ClientToScreen($hwnd, [ref]$from) | Out-Null
        if ((Test-EditorWindowAt $from $processId) -eq $false) {
            Write-Output "Skip drag: no editor window under the start point ($(Get-WindowDescriptionAt $from))"
            return
        }

        # MOUSEEVENTF_RIGHTDOWN 0x08, RIGHTUP 0x10. 여러 프레임에 걸쳐 조금씩 옮긴다.
        [GfwVerifyWin32]::SetCursorPos($from.X, $from.Y) | Out-Null
        Start-Sleep -Milliseconds 300
        [GfwVerifyWin32]::mouse_event(0x08, 0, 0, 0, [UIntPtr]::Zero)
        Start-Sleep -Milliseconds 100
        $steps = 20
        for ($s = 1; $s -le $steps; $s++) {
            $point = New-Object GfwVerifyWin32+POINT
            $point.X = [int]($x1 + ($x2 - $x1) * $s / $steps)
            $point.Y = [int]($y1 + ($y2 - $y1) * $s / $steps)
            [GfwVerifyWin32]::ClientToScreen($hwnd, [ref]$point) | Out-Null
            [GfwVerifyWin32]::SetCursorPos($point.X, $point.Y) | Out-Null
            Start-Sleep -Milliseconds 30
        }
        [GfwVerifyWin32]::mouse_event(0x10, 0, 0, 0, [UIntPtr]::Zero)
        Write-Output "Right-dragged client ($x1, $y1) -> ($x2, $y2)"
    }
    finally {
        [GfwVerifyWin32]::SetThreadDpiAwarenessContext($previous) | Out-Null
    }
}

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

Start-Sleep -Seconds $WaitSeconds
$proc.Refresh()
if ($proc.HasExited) { Write-Output "Process exited during wait with code $($proc.ExitCode)"; exit 4 }

$client = New-Object GfwVerifyWin32+RECT
[GfwVerifyWin32]::GetClientRect($hwnd, [ref]$client) | Out-Null
$awareness = [GfwVerifyWin32]::GetAwarenessFromDpiAwarenessContext([GfwVerifyWin32]::GetWindowDpiAwarenessContext($hwnd))
Write-Output "Client area (physical) $($client.Right)x$($client.Bottom), window DPI awareness $awareness (0 unaware, 1 system, 2 per monitor)"

Save-WindowCapture $hwnd $proc.Id "${OutPrefix}_before.png"

if ($Clicks -ne "" -or $RightDrag -ne "") {
    $saved = New-Object GfwVerifyWin32+POINT
    [GfwVerifyWin32]::GetCursorPos([ref]$saved) | Out-Null
    [GfwVerifyWin32]::SetWindowPos($hwnd, $HWND_TOPMOST, 0, 0, 0, 0, $SWP_NOSIZE_NOMOVE_SHOW) | Out-Null
    Start-Sleep -Milliseconds 500
    try {
        $index = 0
        if ($Clicks -ne "") {
            foreach ($pair in $Clicks.Split(';')) {
                $xy = $pair.Split(',')
                Invoke-ClientClick $hwnd $proc.Id ([int]$xy[0]) ([int]$xy[1])
                Start-Sleep -Seconds 2
                $index++
                Save-WindowCapture $hwnd $proc.Id "${OutPrefix}_click$index.png"
            }
        }
        if ($RightDrag -ne "") {
            $d = $RightDrag.Split(',')
            Invoke-ClientRightDrag $hwnd $proc.Id ([int]$d[0]) ([int]$d[1]) ([int]$d[2]) ([int]$d[3])
            Start-Sleep -Seconds 2
            Save-WindowCapture $hwnd $proc.Id "${OutPrefix}_rightdrag.png"
        }
    }
    finally {
        [GfwVerifyWin32]::SetCursorPos($saved.X, $saved.Y) | Out-Null
        [GfwVerifyWin32]::SetWindowPos($hwnd, $HWND_NOTOPMOST, 0, 0, 0, 0, $SWP_NOSIZE_NOMOVE_SHOW) | Out-Null
    }
}

[GfwVerifyWin32]::PostMessage($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
if (-not $proc.WaitForExit(60000)) { Write-Output "Timeout waiting exit, killing"; $proc.Kill(); exit 5 }
Write-Output "ExitCode $($proc.ExitCode)"
exit 0
