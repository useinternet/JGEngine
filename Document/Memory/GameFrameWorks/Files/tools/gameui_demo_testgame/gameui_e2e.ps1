param(
    [string]$BinDir,
    [string]$OutDir,
    [int]$WaitSeconds = 25
)
# 게임 UI(JGGameWidget, CommonUI식) 화면 검증(V2). ER-007 줄바꿈 패널 · ER-008 입력란 단계 포함. 게임 프로젝트 Bin 의 런처를 띄워 에디터 Scene Viewport 의 HUD · 메뉴를 실제 입력으로 확인한다.
#  1) 캡처(before) → 주황 End Turn 버튼을 색으로 찾아 뷰포트 이미지 사각형을 역산
#  2) 기준 해상도(1920x1080) 좌표로 클릭 · Esc 를 PostMessage 로 보낸다(런처 창에만, 전경 창 · 커서를 바꾸지 않는다)
#     End Turn → Menu → (메뉴 캡처) → 메뉴가 떠 있는 동안 End Turn · 빈 곳 클릭(막혀야 함) → Esc(메뉴 닫힘) → End Turn → 빈 곳(월드 피킹)
#     → 비활성 Draw 버튼 → Menu → Resume(메뉴 닫힘)
#     → 주소 입력란 클릭 · 글자(WM_CHAR) · Backspace · Enter(확정) → 이름 입력란 · 한글 · (입력 캡처) · Enter → 다시 입력 중 Esc(포커스만 해제) → Esc(뒤로가기)
#  3) 캡처(after) → WM_CLOSE → 종료 코드. 결과 판정은 게임 Bin 의 jg_log.txt 로 한다(README 기대 결과).
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class GUIE2E {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X; public int Y; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT lpPoint);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdcBlt, uint nFlags);
    // 유니코드 판(PostMessageW). 문자 집합을 정하지 않으면 PostMessageA 로 묶여 WM_CHAR 의 한글이 코드 페이지로 바뀌어 깨진다(10-01 겪음).
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
}
"@

function Capture([IntPtr]$hwnd, [string]$path) {
    $rect = New-Object GUIE2E+RECT
    [GUIE2E]::GetWindowRect($hwnd, [ref]$rect) | Out-Null
    $w = $rect.Right - $rect.Left; $h = $rect.Bottom - $rect.Top
    $bmp = New-Object System.Drawing.Bitmap($w, $h)
    $gfx = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $gfx.GetHdc()
    [GUIE2E]::PrintWindow($hwnd, $hdc, 2) | Out-Null   # PW_RENDERFULLCONTENT
    $gfx.ReleaseHdc($hdc); $gfx.Dispose()
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    return $bmp
}

# End Turn 보통 색(선형 0.95, 0.45, 0.10)이 화면에 보이는 색 = sRGB (249, 179, 89).
# 출력 텍스처(FP16)가 선형으로 표시돼 sRGB 로 바뀐다(ImGui 색도 같다, 색 공간은 Graphics D-3). ±6 안의 픽셀을 모으고,
# 카드 그림 같은 곳의 드문 비슷한 픽셀을 버리려고 중앙값 둘레(±200 px)만 경계 상자에 넣는다. 창 좌표(창 왼쪽 위 기준).
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
            $b = $bytes[$i]; $g = $bytes[$i + 1]; $r = $bytes[$i + 2]
            if ([Math]::Abs($r - 249) -le 6 -and [Math]::Abs($g - 179) -le 6 -and [Math]::Abs($b - 89) -le 6) {
                $xs.Add($x); $ys.Add($y)
            }
        }
    }
    if ($xs.Count -eq 0) { return @{ Count = 0; MinX = -1; MinY = -1; MaxX = -1; MaxY = -1 } }
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

function Post-Click([IntPtr]$hwnd, [int]$clientX, [int]$clientY) {
    $lParam = [IntPtr](($clientY -shl 16) -bor ($clientX -band 0xFFFF))
    # 세 메시지를 잇달아 넣는다(실제 커서가 창 밖이면 곧 WM_MOUSELEAVE 가 오므로). ImGui 입력 큐가 프레임을 나눠 처리한다.
    [GUIE2E]::PostMessage($hwnd, 0x0200, [IntPtr]::Zero, $lParam) | Out-Null   # WM_MOUSEMOVE
    [GUIE2E]::PostMessage($hwnd, 0x0201, [IntPtr]1, $lParam) | Out-Null         # WM_LBUTTONDOWN
    [GUIE2E]::PostMessage($hwnd, 0x0202, [IntPtr]::Zero, $lParam) | Out-Null    # WM_LBUTTONUP
    Start-Sleep -Milliseconds 800
}

function Post-Key([IntPtr]$hwnd, [int]$vk, [int64]$downLParam, [int64]$upLParam) {
    # 뗌은 이전 상태 · 전이 비트(30, 31)를 켠다. 스캔 코드는 16~23 비트.
    [GUIE2E]::PostMessage($hwnd, 0x0100, [IntPtr]$vk, [IntPtr]$downLParam) | Out-Null   # WM_KEYDOWN (TranslateMessage 가 WM_CHAR 도 만든다)
    Start-Sleep -Milliseconds 150
    [GUIE2E]::PostMessage($hwnd, 0x0101, [IntPtr]$vk, [IntPtr]$upLParam) | Out-Null     # WM_KEYUP
    Start-Sleep -Milliseconds 800
}

function Post-Escape([IntPtr]$hwnd) {
    Post-Key $hwnd 0x1B ([int64]0x00010001) ([int64]0xC0010001)   # Esc: 스캔 코드 0x01
}

function Post-Backspace([IntPtr]$hwnd) {
    Post-Key $hwnd 0x08 ([int64]0x000E0001) ([int64]0xC00E0001)   # Backspace: 스캔 코드 0x0E
}

function Post-Enter([IntPtr]$hwnd) {
    Post-Key $hwnd 0x0D ([int64]0x001C0001) ([int64]0xC01C0001)   # Enter: 스캔 코드 0x1C (확장 비트 없음 = 본 키보드 Enter)
}

function Post-Text([IntPtr]$hwnd, [string]$text) {
    # 확정 글자를 WM_CHAR(UTF-16)로 넣는다(IME 확정 글자와 같은 길). 실제 IME 조합은 사람이 본다.
    foreach ($ch in $text.ToCharArray()) {
        [GUIE2E]::PostMessage($hwnd, 0x0102, [IntPtr][int]$ch, [IntPtr]1) | Out-Null   # WM_CHAR
    }
    Start-Sleep -Milliseconds 800
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
Write-Output "Main window: $hwnd, title: $($proc.MainWindowTitle)"
Start-Sleep -Seconds $WaitSeconds

$winRect = New-Object GUIE2E+RECT
[GUIE2E]::GetWindowRect($hwnd, [ref]$winRect) | Out-Null
$origin = New-Object GUIE2E+POINT
[GUIE2E]::ClientToScreen($hwnd, [ref]$origin) | Out-Null
$offX = $origin.X - $winRect.Left; $offY = $origin.Y - $winRect.Top   # 창 좌표 → 클라이언트 좌표

$before = Capture $hwnd (Join-Path $OutDir "gameui_e2e_before.png")
$found = Find-Orange $before
$before.Dispose()
Write-Output ("Orange pixels {0}, box ({1},{2})-({3},{4})" -f $found.Count, $found.MinX, $found.MinY, $found.MaxX, $found.MaxY)
if ($found.Count -lt 300) { Write-Output "End Turn button not found in the capture"; [GUIE2E]::PostMessage($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null; $proc.WaitForExit(30000) | Out-Null; exit 4 }

# 버튼 폭으로 뷰포트 이미지 사각형을 거꾸로 구한다(End Turn 340x110, 오른쪽 · 아래 여백 40, 기준 1920x1080).
# 세로는 버튼 높이 대신 렌더 타깃 비율(Scene Viewport 1280x720 = 16:9)로 구한다 — 높이는 가장자리 픽셀 1~2 개가 색 범위를 벗어나
# 3% 가량 짧게 잡히고, 작은 버튼(Menu, 높이 64)을 누를 때 그만큼 빗나간다(10-01 첫 실행에서 겪음).
$bw = $found.MaxX - $found.MinX + 1
$imageW = $bw * 1920.0 / 340.0; $imageH = $imageW * 720.0 / 1280.0
$imageRight  = $found.MaxX + $imageW * 40.0 / 1920.0
$imageBottom = $found.MaxY + $imageH * 40.0 / 1080.0
$imageLeft = $imageRight - $imageW; $imageTop = $imageBottom - $imageH
Write-Output ("Viewport image estimate ({0:N0},{1:N0})-({2:N0},{3:N0})" -f $imageLeft, $imageTop, $imageRight, $imageBottom)

# 기준 해상도 좌표 → 클라이언트 좌표
function Click-Ref([string]$label, [double]$refX, [double]$refY) {
    $cx = [int]($imageLeft + $imageW * $refX / 1920.0) - $offX
    $cy = [int]($imageTop + $imageH * $refY / 1080.0) - $offY
    Write-Output ("{0} at ref ({1}, {2}) -> client ({3}, {4})" -f $label, $refX, $refY, $cx, $cy)
    Post-Click $hwnd $cx $cy
}

# HUD 배치(JGGameUIDemoHUD): End Turn 가운데 (1710, 985) · Menu (1770, 48) · Draw(비활성) (1400, 985)
# 일시정지 메뉴(JGGameUIDemoPauseMenu): Resume (960, 640). 빈 곳 = (480, 540) (메뉴 패널 왼쪽, 월드)
Click-Ref "Step 1: End Turn" 1710 985
Click-Ref "Step 2: Menu (push pause menu)" 1770 48
Start-Sleep -Seconds 1
$menuShot = Capture $hwnd (Join-Path $OutDir "gameui_e2e_menu.png")
$menuShot.Dispose()
Click-Ref "Step 3: End Turn while the menu is up (blocked)" 1710 985
Click-Ref "Step 4: empty area while the menu is up (blocked)" 480 540
Write-Output "Step 5: Esc (back -> close the menu)"
Post-Escape $hwnd
Click-Ref "Step 6: End Turn" 1710 985
Click-Ref "Step 7: empty area (world picking)" 480 540
Click-Ref "Step 8: disabled Draw button (blocked, no click)" 1400 985
Click-Ref "Step 9: Menu again" 1770 48
Click-Ref "Step 10: Resume (close the menu)" 960 640
Start-Sleep -Seconds 1

# 글자 입력(ER-008). 입력란: 주소 가운데 (1560, 630) · 이름 (1560, 710). 줄바꿈 패널은 그 위(오른쪽 위).
Click-Ref "Step 11: Address input (text focus)" 1560 630
Write-Output "Step 12: type 192.168.0.10:47771x ('x' is not allowed)"
Post-Text $hwnd "192.168.0.10:47771x"
Write-Output "Step 13: Backspace"
Post-Backspace $hwnd
Write-Output "Step 14: Enter (commit -> 192.168.0.10:4777)"
Post-Enter $hwnd
Click-Ref "Step 15: Name input (text focus)" 1560 710
$hangulName = -join @([char]0xD64D, [char]0xAE38, [char]0xB3D9)   # 홍길동
Write-Output "Step 16: type Hangul name + ab"
Post-Text $hwnd ($hangulName + "ab")
$inputShot = Capture $hwnd (Join-Path $OutDir "gameui_e2e_input.png")
$inputShot.Dispose()
Write-Output "Step 17: Enter (commit)"
Post-Enter $hwnd
Click-Ref "Step 18: Name input again" 1560 710
Post-Text $hwnd "z"
Write-Output "Step 19: Esc while typing (only releases the text focus: no commit, no back)"
Post-Escape $hwnd
Write-Output "Step 20: Esc again (back: the HUD has no back handler)"
Post-Escape $hwnd

$after = Capture $hwnd (Join-Path $OutDir "gameui_e2e_after.png")
$after.Dispose()

[GUIE2E]::PostMessage($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null   # WM_CLOSE
if (-not $proc.WaitForExit(30000)) { Write-Output "Timeout waiting exit, killing"; $proc.Kill(); exit 5 }
Write-Output "ExitCode $($proc.ExitCode)"
exit 0
