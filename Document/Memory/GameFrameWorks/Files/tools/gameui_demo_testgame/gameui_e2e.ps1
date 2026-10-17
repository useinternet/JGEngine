param(
    [string]$BinDir,
    [string]$OutDir,
    [int]$WaitSeconds = 40,   # 시작 뒤 화면이 자리 잡을 때까지
    [switch]$NoCheck,         # 끝에 게임 로그로 단계마다 판정하지 않는다(이 데모 HUD 가 아닐 때)
    [string]$CheckLog         # 띄우지 않고 이 로그만 판정한다(지난 실행의 jg_log.txt 다시 보기)
)
# 게임 UI(JGGameWidget, CommonUI식) 화면 검증(V2). ER-007 줄바꿈 패널 · ER-008 입력란 단계 포함. 게임 프로젝트 Bin 의 런처를 띄워 에디터 Scene Viewport 의 HUD · 메뉴를 실제 입력으로 확인한다.
#  1) 캡처(before) → 주황 End Turn 버튼을 색으로 찾아 뷰포트 이미지 사각형을 역산
#  2) 기준 해상도(1920x1080) 좌표로 클릭 · Esc 를 PostMessage 로 보낸다(런처 창에만, 전경 창 · 커서를 바꾸지 않는다)
#     End Turn → Menu → (메뉴 캡처) → 메뉴가 떠 있는 동안 End Turn · 빈 곳 클릭(막혀야 함) → Esc(메뉴 닫힘) → End Turn → 빈 곳(월드 피킹)
#     → 비활성 Draw 버튼 → Menu → Resume(메뉴 닫힘)
#     → 주소 입력란 클릭 · 글자(WM_CHAR) · Backspace · Enter(확정) → 이름 입력란 · 한글 · (입력 캡처) · Enter → 다시 입력 중 Esc(포커스만 해제) → Esc(뒤로가기)
#     → 끌어다 놓기(GG-15): 금색 카드를 놓을 곳으로 끌어 놓기 → 다시 끌어 뷰포트 이미지 밖에서 떼기(놓은 곳 없음 → 제자리)
#     → 열림 · 닫힘 전환(GG-16): 메뉴 열기 → Esc 바로 뒤 Resume 자리 클릭(닫히는 메뉴는 받지 않음 → 월드) · 닫히는 중 캡처 → 다시 열기 → Esc 바로 뒤 Menu(닫히다 다시 열기) → Resume
#     → 스크롤 칸(GG-20, 28~37): 목록 칸 휠 3칸 → 줄 클릭 → 칸 아래(잘린 줄 자리) 클릭(월드) → 세로 손잡이 끝까지 끌기 → 세로 막대 빈 곳 → 위 띠 위 휠(넘김 없음)
#       → 가로 손잡이 끝까지 끌기 → 가로 휠(WM_MOUSEHWHEEL) → 이야기 칸 휠 2칸(부드럽게) → 이야기 막대 빈 곳 → (스크롤 캡처)
#     → 휠 받기(GG-21, 38~41): 지도 휠 앞으로 2칸(확대) → 다른 자리에서 뒤로 1칸(축소) → 지도 끌어 옮기기 → 끄는 중 휠(끌면서 확대) → (지도 캡처)
#  3) 캡처(after) → WM_CLOSE → 종료 코드 → 게임 Bin 의 jg_log.txt 로 단계마다 판정(README 기대 결과, -NoCheck 면 건너뜀).
#     모든 단계 통과면 "E2E CHECK: PASS" 와 종료 코드 0, 하나라도 빠지면 "E2E CHECK: FAIL" · 빠진 단계와 종료 코드 6.
# 입력 넣기(GG-H32, 10-11): 자리가 필요한 입력(이동 + 누름 · 뗌, 이동 + 휠)은 C# 안에서 한 묶음으로 잇달아 넣고, 넣기 전에 앱 프레임에 맞춘다.
#   PowerShell 줄마다 PostMessage 하면 사이 간격(줄마다 0.1ms 넘게, 처음 부를 때는 더)에 앱이 앞 메시지를 먼저 꺼낼 수 있다 — 이동을 처리할 때
#   ImGui Win32 백엔드가 건 TrackMouseEvent 의 WM_MOUSELEAVE 가(실제 커서가 창 밖이면 곧바로) 뒤 메시지보다 먼저 처리되어,
#   마우스 자리가 무효인 채 누름 · 뗌 · 휠이 들어가 빠졌다(회차마다 한두 개). 이동 뒤 30ms 쉬고 누름 · 뗌을 넣으면 30번 모두 빠진다.
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
public class GUIE2E {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X; public int Y; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT lpPoint);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdcBlt, uint nFlags);
    // 유니코드 판(PostMessageW). 문자 집합을 정하지 않으면 PostMessageA 로 묶여 WM_CHAR 의 한글이 코드 페이지로 바뀌어 깨진다(10-01 겪음).
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] public static extern IntPtr GetWindowDpiAwarenessContext(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern int GetAwarenessFromDpiAwarenessContext(IntPtr value);
    [DllImport("user32.dll")] public static extern IntPtr SetThreadDpiAwarenessContext(IntPtr dpiContext);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern IntPtr SendMessageTimeout(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam, uint fuFlags, uint uTimeout, out IntPtr lpdwResult);

    // 앱 프레임에 맞춘다: WM_NULL 을 보내 앱 스레드가 메시지를 꺼낼 때(프레임 앞머리 PWindowsJWindow::Update)까지 기다린다.
    // 돌아오면 앱은 그 프레임의 갱신 · 그리기로 넘어가므로, 곧 넣은 메시지는 다음 프레임 앞머리에서 한꺼번에 꺼내진다. 돌려주는 값 = 기다린 ms(시간 초과 -1).
    public static double SyncFrame(IntPtr hwnd)
    {
        IntPtr result;
        Stopwatch watch = Stopwatch.StartNew();
        IntPtr ok = SendMessageTimeout(hwnd, 0x0000, IntPtr.Zero, IntPtr.Zero, 0x0002, 2000, out result);   // WM_NULL, SMTO_ABORTIFHUNG
        if (ok == IntPtr.Zero)
        {
            return -1.0;
        }
        return watch.Elapsed.TotalMilliseconds;
    }

    // 자리가 필요한 묶음(이동 + 누름 · 뗌 · 휠)을 프레임에 맞춘 뒤 3ms 쉬고 잇달아 넣는다(사이 간격 수 µs — 앱이 묶음 가운데를 먼저 꺼내지 못한다).
    public static double PostBurst(IntPtr hwnd, uint[] messages, long[] wParams, long[] lParams)
    {
        double waited = SyncFrame(hwnd);
        System.Threading.Thread.Sleep(3);
        for (int i = 0; i < messages.Length; ++i)
        {
            PostMessage(hwnd, messages[i], (IntPtr)wParams[i], (IntPtr)lParams[i]);
        }
        return waited;
    }
}
"@

# 묶음마다 프레임에 맞추며 기다린 시간(ms) — 끝에 요약한다. -1 은 시간 초과(그래도 넣는다).
$script:BurstWaits = New-Object System.Collections.Generic.List[double]

function Make-LParam([int]$x, [int]$y) {
    return [int64](($y -shl 16) -bor ($x -band 0xFFFF))
}

function Post-Burst([uint32[]]$messages, [int64[]]$wParams, [int64[]]$lParams) {
    $waited = [GUIE2E]::PostBurst($hwnd, $messages, $wParams, $lParams)
    $script:BurstWaits.Add($waited)
}

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

# End Turn 보통 색(코드 값 0.95, 0.45, 0.10)이 화면에 보이는 색은 출력 색 공간에 따라 다르다(Graphics D-3 · 5-14):
#   5-14 뒤 — 게임 UI 를 8비트 디스플레이 텍스처(sRGB 인코딩 값)에 그려 코드 값 그대로 = (242, 115, 26)
#   5-14 전 — 선형 FP16 출력 텍스처에 그려 화면에서 sRGB 로 한 번 더 바뀜 = (249, 179, 89)
# 두 색을 차례로 찾는다. ±6 안의 픽셀을 모으고, 카드 그림 같은 곳의 드문 비슷한 픽셀을 버리려고
# 중앙값 둘레(±200 px)만 경계 상자에 넣는다. 창 좌표(창 왼쪽 위 기준).
function Find-Color([System.Drawing.Bitmap]$bmp, [int]$targetR, [int]$targetG, [int]$targetB) {
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
            if ([Math]::Abs($r - $targetR) -le 6 -and [Math]::Abs($g - $targetG) -le 6 -and [Math]::Abs($b - $targetB) -le 6) {
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
    # [이동 · 누름 · 뗌]을 한 묶음으로(앞머리 설명). ImGui 입력 큐가 누름 · 뗌을 두 프레임에 나눠 처리한다.
    $lParam = Make-LParam $clientX $clientY
    Post-Burst @(0x0200, 0x0201, 0x0202) @(0, 1, 0) @($lParam, $lParam, $lParam)   # WM_MOUSEMOVE · WM_LBUTTONDOWN · WM_LBUTTONUP
    Start-Sleep -Milliseconds 800
}

function Post-Key([IntPtr]$hwnd, [int]$vk, [int64]$downLParam, [int64]$upLParam) {
    # 뗌은 이전 상태 · 전이 비트(30, 31)를 켠다. 스캔 코드는 16~23 비트.
    [GUIE2E]::PostMessage($hwnd, 0x0100, [IntPtr]$vk, [IntPtr]$downLParam) | Out-Null   # WM_KEYDOWN (TranslateMessage 가 WM_CHAR 도 만든다)
    Start-Sleep -Milliseconds 150
    [GUIE2E]::PostMessage($hwnd, 0x0101, [IntPtr]$vk, [IntPtr]$upLParam) | Out-Null     # WM_KEYUP
    Start-Sleep -Milliseconds 800
}

# 닫힘 전환(0.4초) 안에 다음 입력을 넣으려고 기다리지 않는 판(누름 → 30ms → 뗌, 뒤에 기다리지 않음).
function Post-EscapeQuick([IntPtr]$hwnd) {
    [GUIE2E]::PostMessage($hwnd, 0x0100, [IntPtr]0x1B, [IntPtr]([int64]0x00010001)) | Out-Null   # WM_KEYDOWN Esc
    Start-Sleep -Milliseconds 30
    [GUIE2E]::PostMessage($hwnd, 0x0101, [IntPtr]0x1B, [IntPtr]([int64]0xC0010001)) | Out-Null   # WM_KEYUP
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

# 로그는 실행마다 처음부터 다시 쓰인다(Core Log). Info 줄은 종료할 때 다 쓰이므로 실행이 끝난 뒤에 읽는다.
function Read-LogFile([string]$path) {
    $stream = [System.IO.File]::Open($path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
    $reader = New-Object System.IO.StreamReader($stream, [System.Text.Encoding]::UTF8)
    $text = $reader.ReadToEnd()
    $reader.Dispose()
    return $text
}

# 판정 결과를 쓰고 종료 코드를 돌려준다(0 = 통과, 6 = 빠진 단계 있음).
function Invoke-E2ECheck([string]$log, [int]$exitCode) {
    # ---- 단계마다 판정(README 기대 결과). 차례 검사: 각 줄은 앞 줄을 찾은 자리 뒤에서 찾는다. None = 앞 줄과 이 줄 사이에 있으면 안 되는 것.
    $uiClick    = "SceneViewport Click \([^)]*\): game UI \(not picked\)"
    $worldClick = "SceneViewport Click \([^)]*\): nothing \(no controller in the world\)"
    $backByUI   = "SceneViewport Back \(Esc\): handled by game UI"
    $hangulName = -join @([char]0xD64D, [char]0xAE38, [char]0xB3D9)   # 홍길동
    $expected = @(
        @{ Step = "1";  Text = "End Turn clicked \(turn 2\)" },
        @{ Step = "2";  Text = "GameUIDemo Menu clicked" },
        @{ Step = "2";  Text = "GameUI push JGGameUIDemoPauseMenu on Menu \(stack 1\)" },
        @{ Step = "3";  Text = $uiClick },
        @{ Step = "4";  Text = $uiClick },
        @{ Step = "5";  Text = $backByUI; None = "End Turn clicked" },
        @{ Step = "6";  Text = "End Turn clicked \(turn 3\)" },
        @{ Step = "7";  Text = $worldClick },
        @{ Step = "8";  Text = $uiClick },
        @{ Step = "9";  Text = "GameUIDemo Menu clicked" },
        @{ Step = "10"; Text = "GameUIDemo Resume clicked" },
        @{ Step = "11"; Text = $uiClick },
        @{ Step = "12-14"; Text = "address committed: 192\.168\.0\.10:4777\r?$" },
        @{ Step = "15"; Text = $uiClick },
        @{ Step = "16-17"; Text = ("name committed: " + $hangulName + "ab\r?$") },
        @{ Step = "18"; Text = $uiClick },
        @{ Step = "19-20"; Text = "SceneViewport Back \(Esc\): no game UI back handler"; None = "name committed" },
        @{ Step = "21"; Text = "drag end Card1: dropped on DropZone" },
        @{ Step = "22"; Text = "drag end Card1: returned, drop target none" },
        @{ Step = "23"; Text = "GameUIDemo Menu clicked" },
        @{ Step = "24"; Text = $backByUI },
        @{ Step = "24"; Text = $worldClick; None = "Resume clicked" },
        @{ Step = "24"; Text = "GameUI closed JGGameUIDemoPauseMenu on Menu" },
        @{ Step = "25"; Text = "GameUIDemo Menu clicked" },
        @{ Step = "26"; Text = $backByUI },
        @{ Step = "26"; Text = "GameUI push JGGameUIDemoPauseMenu on Menu \(stack 1\) - reopened while closing"; None = "pause menu closed" },
        @{ Step = "27"; Text = "GameUIDemo Resume clicked" },
        @{ Step = "28"; Text = "scroll ListScroll offset \(0\.0, 60\.0\)" },
        @{ Step = "28"; Text = "scroll ListScroll offset \(0\.0, 120\.0\)" },
        @{ Step = "28"; Text = "scroll ListScroll offset \(0\.0, 180\.0\)" },
        @{ Step = "29"; Text = "list row 5 clicked" },
        @{ Step = "30"; Text = $worldClick; None = "list row" },
        @{ Step = "31"; Text = "scroll ListScroll offset \(0\.0, 1361\.0\)" },
        @{ Step = "32"; Text = "scroll ListScroll offset \(0\.0, 1155\.4\)" },
        @{ Step = "33-34"; Text = "scroll ListScroll offset \(106\.0, 1155\.4\)"; None = "scroll (ListScroll|StoryScroll) offset \(0\.0, " },
        @{ Step = "35"; Text = "scroll ListScroll offset \(46\.0, 1155\.4\)" },
        @{ Step = "36"; Text = "scroll StoryScroll offset \(0\.0, 60\.0\)" },
        @{ Step = "36"; Text = "scroll StoryScroll offset \(0\.0, 120\.0\)" },
        @{ Step = "37"; Text = "scroll StoryScroll offset \(0\.0, 242\.5\)" },
        @{ Step = "38"; Text = "map zoom wheel 1\.00 -> scale 0\.575" },
        @{ Step = "38"; Text = "map zoom done scale 0\.575, point under the pointer moved 0\.00" },
        @{ Step = "38"; Text = "map zoom wheel 1\.00 -> scale 0\.661" },
        @{ Step = "38"; Text = "map zoom done scale 0\.661, point under the pointer moved 0\.00" },
        @{ Step = "39"; Text = "map zoom wheel -1\.00 -> scale 0\.575" },
        @{ Step = "39"; Text = "map zoom done scale 0\.575, point under the pointer moved 0\.00" },
        @{ Step = "40"; Text = "map pan begin at" },
        @{ Step = "40"; Text = "map pan end at \([^)]*\): translation \([^)]*\), scale 0\.575" },
        @{ Step = "41"; Text = "map pan begin at" },
        @{ Step = "41"; Text = "map zoom wheel 1\.00 -> scale 0\.661 at \([^)]*\) \(dragging\)" },
        @{ Step = "41"; Text = "map zoom done scale 0\.661, point under the pointer moved 0\.00" },
        @{ Step = "41"; Text = "map pan end at \([^)]*\): translation \([^)]*\), scale 0\.661" }
    )

    $failures = New-Object System.Collections.Generic.List[string]
    $position = 0
    foreach ($item in $expected) {
        $match = [regex]::new($item.Text, [System.Text.RegularExpressions.RegexOptions]::Multiline).Match($log, $position)
        if ($match.Success -eq $false) {
            $failures.Add(("step {0}: not found after the previous step - {1}" -f $item.Step, $item.Text))
            continue
        }
        if ($item.ContainsKey("None")) {
            $between = $log.Substring($position, $match.Index - $position)
            if ([regex]::IsMatch($between, $item.None)) {
                $failures.Add(("step {0}: unexpected '{1}' before it" -f $item.Step, $item.None))
            }
        }
        $position = $match.Index + $match.Length
    }

    # 모두 세는 것(차례와 상관없이)
    $counts = @(
        @{ Step = "1-27"; Text = "End Turn clicked"; Count = 2 },
        @{ Step = "2-26"; Text = "GameUIDemo Menu clicked"; Count = 5 },
        @{ Step = "10-27"; Text = "GameUIDemo Resume clicked"; Count = 2 },
        @{ Step = "8";  Text = "Draw clicked while disabled"; Count = 0 },
        @{ Step = "19"; Text = ("name committed: " + $hangulName + "abz"); Count = 0 },
        @{ Step = "all"; Text = "\[(error|critical)\]"; Count = 0 },
        @{ Step = "all"; Text = "live blocks [1-9]"; Count = 0 }
    )
    foreach ($item in $counts) {
        $found = [regex]::Matches($log, $item.Text).Count
        if ($found -ne $item.Count) {
            $failures.Add(("step {0}: '{1}' {2} times (expected {3})" -f $item.Step, $item.Text, $found, $item.Count))
        }
    }
    if ($exitCode -ne 0) {
        $failures.Add(("exit code {0} (expected 0)" -f $exitCode))
    }

    if ($failures.Count -eq 0) {
        Write-Host ("E2E CHECK: PASS - steps 1-41, {0} ordered checks + {1} counts" -f $expected.Count, $counts.Count)
        return 0
    }
    Write-Host ("E2E CHECK: FAIL - {0} problems" -f $failures.Count)
    foreach ($failure in $failures) {
        Write-Host ("  " + $failure)
    }
    return 6
}

if ($CheckLog) {
    exit (Invoke-E2ECheck (Read-LogFile $CheckLog) 0)
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
# 에디터가 DPI 인식(D-22 ②, 0 = 인식 안 함 · 1 시스템 · 2 모니터별)이면 이 스레드도 모니터별 인식으로 바꾼다 — 창 사각형 · 캡처 · PostMessage 좌표가
# 모두 실제 픽셀이 된다. 인식 안 하는 옛 빌드면 그대로 둔다(그때는 앱 좌표 = 논리 좌표라 지금까지처럼 맞는다).
$windowAwareness = [GUIE2E]::GetAwarenessFromDpiAwarenessContext([GUIE2E]::GetWindowDpiAwarenessContext($hwnd))
if ($windowAwareness -ne 0) { [GUIE2E]::SetThreadDpiAwarenessContext([IntPtr](-4)) | Out-Null }   # DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
Write-Output "Window DPI awareness $windowAwareness"
Start-Sleep -Seconds $WaitSeconds

$winRect = New-Object GUIE2E+RECT
[GUIE2E]::GetWindowRect($hwnd, [ref]$winRect) | Out-Null
$origin = New-Object GUIE2E+POINT
[GUIE2E]::ClientToScreen($hwnd, [ref]$origin) | Out-Null
$offX = $origin.X - $winRect.Left; $offY = $origin.Y - $winRect.Top   # 창 좌표 → 클라이언트 좌표

$before = Capture $hwnd (Join-Path $OutDir "gameui_e2e_before.png")
$found = Find-Color $before 242 115 26
$orangeLabel = "sRGB display (242,115,26)"
if ($found.Count -lt 300) {
    $found = Find-Color $before 249 179 89
    $orangeLabel = "linear output (249,179,89)"
}
$before.Dispose()
Write-Output ("Orange pixels {0} as {1}, box ({2},{3})-({4},{5})" -f $found.Count, $orangeLabel, $found.MinX, $found.MinY, $found.MaxX, $found.MaxY)
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
function Ref-ToClient([double]$refX, [double]$refY) {
    $cx = [int]($imageLeft + $imageW * $refX / 1920.0) - $offX
    $cy = [int]($imageTop + $imageH * $refY / 1080.0) - $offY
    return @($cx, $cy)
}

function Click-Ref([string]$label, [double]$refX, [double]$refY) {
    $client = Ref-ToClient $refX $refY
    Write-Output ("{0} at ref ({1}, {2}) -> client ({3}, {4})" -f $label, $refX, $refY, $client[0], $client[1])
    Post-Click $hwnd $client[0] $client[1]
}

# 기다리지 않는 클릭(묶음을 넣기만 한다 — 닫힘 전환 안에서 누르기. 프레임 맞추기는 한 프레임(약 30ms) 안).
function Click-RefQuick([string]$label, [double]$refX, [double]$refY) {
    $client = Ref-ToClient $refX $refY
    Write-Output ("{0} at ref ({1}, {2}) -> client ({3}, {4})" -f $label, $refX, $refY, $client[0], $client[1])
    $lParam = Make-LParam $client[0] $client[1]
    Post-Burst @(0x0200, 0x0201, 0x0202) @(0, 1, 0) @($lParam, $lParam, $lParam)
}

function Post-Mouse([IntPtr]$hwnd, [int]$msg, [int]$keys, [int]$clientX, [int]$clientY) {
    $lParam = [IntPtr](($clientY -shl 16) -bor ($clientX -band 0xFFFF))
    [GUIE2E]::PostMessage($hwnd, $msg, [IntPtr]$keys, $lParam) | Out-Null
}

# 휠(GG-20): 한 칸마다 [이동 · 휠]을 잇달아 넣는다(이동이 먼저 처리돼 휠이 그 자리에서 온다).
# wParam 위 16비트 = 한 칸 120 — 세로(WM_MOUSEWHEEL 0x020A)는 앞으로 +, 뒤로(아래로 넘기기) −. 가로(WM_MOUSEHWHEEL 0x020E)는 오른쪽 +.
# lParam 은 화면 좌표다(클라이언트 + 클라이언트 원점).
# 끄는 중 휠(GG-21): [이동 · 누름] → 문턱(6)을 넘는 이동 몇 번 → [이동 · 휠(왼쪽 누름 상태)] → [이동 · 뗌].
function DragWheel-Ref([string]$label, [double]$fromX, [double]$fromY, [double]$wheelX, [double]$wheelY, [double]$toX, [double]$toY, [int]$notches) {
    $from = Ref-ToClient $fromX $fromY
    $mid  = Ref-ToClient $wheelX $wheelY
    $to   = Ref-ToClient $toX $toY
    Write-Output ("{0} from ref ({1}, {2}), wheel {3} at ({4}, {5}), release at ({6}, {7})" -f $label, $fromX, $fromY, $notches, $wheelX, $wheelY, $toX, $toY)
    $fromLParam = Make-LParam $from[0] $from[1]
    Post-Burst @(0x0200, 0x0201) @(0, 1) @($fromLParam, $fromLParam)   # [WM_MOUSEMOVE · WM_LBUTTONDOWN]
    Start-Sleep -Milliseconds 300
    for ($s = 1; $s -le 3; $s++) {
        $x = [int]($from[0] + ($mid[0] - $from[0]) * $s / 3)
        $y = [int]($from[1] + ($mid[1] - $from[1]) * $s / 3)
        Post-Mouse $hwnd 0x0200 1 $x $y           # WM_MOUSEMOVE (왼쪽 누름)
        Start-Sleep -Milliseconds 100
    }
    $delta = 120
    if ($notches -lt 0) { $delta = -120 }
    $wParam = [int64]((($delta -band 0xFFFF) -shl 16) -bor 1)   # 위 16비트 = 휠, 아래 = MK_LBUTTON
    $midLParam = Make-LParam $mid[0] $mid[1]
    $screenLParam = Make-LParam ($mid[0] + $origin.X) ($mid[1] + $origin.Y)
    for ($n = 0; $n -lt [Math]::Abs($notches); $n++) {
        Post-Burst @(0x0200, 0x020A) @(1, $wParam) @($midLParam, $screenLParam)   # [WM_MOUSEMOVE · WM_MOUSEWHEEL]
        Start-Sleep -Milliseconds 400
    }
    $toLParam = Make-LParam $to[0] $to[1]
    Post-Burst @(0x0200, 0x0202) @(1, 0) @($toLParam, $toLParam)   # [WM_MOUSEMOVE · WM_LBUTTONUP]
    Start-Sleep -Milliseconds 800
}

function Wheel-Ref([string]$label, [double]$refX, [double]$refY, [int]$notches, [int]$message = 0x020A) {
    $client = Ref-ToClient $refX $refY
    Write-Output ("{0} at ref ({1}, {2}) -> client ({3}, {4}), {5} notches" -f $label, $refX, $refY, $client[0], $client[1], $notches)
    $delta = 120
    if ($notches -lt 0) { $delta = -120 }
    $wParam = [int64](($delta -band 0xFFFF) -shl 16)
    $clientLParam = Make-LParam $client[0] $client[1]
    $screenLParam = Make-LParam ($client[0] + $origin.X) ($client[1] + $origin.Y)
    for ($n = 0; $n -lt [Math]::Abs($notches); $n++) {
        Post-Burst @(0x0200, $message) @(0, $wParam) @($clientLParam, $screenLParam)   # [WM_MOUSEMOVE · 휠]
        Start-Sleep -Milliseconds 400
    }
    Start-Sleep -Milliseconds 400
}

# 끌기: [이동 · 누름] 묶음, 중간 이동 몇 번, [이동 · 뗌] 묶음(wParam MK_LBUTTON = 1).
# 실제 커서가 창 밖이면 보낸 이동마다 곧 WM_MOUSELEAVE 가 따라와 ImGui 마우스 자리가 빈다 — 중간 프레임의 자리는 믿을 수 없고,
# 누름 · 뗌은 바로 앞 이동과 한 묶음이어야 그 자리에서 처리된다. 그래서 결과는 로그(시작 자리 · 놓은 곳)로 본다.
function Drag-Ref([string]$label, [double]$fromX, [double]$fromY, [double]$toX, [double]$toY) {
    $from = Ref-ToClient $fromX $fromY
    $to   = Ref-ToClient $toX $toY
    Write-Output ("{0} from ref ({1}, {2}) to ({3}, {4}) -> client ({5}, {6}) -> ({7}, {8})" -f $label, $fromX, $fromY, $toX, $toY, $from[0], $from[1], $to[0], $to[1])
    $fromLParam = Make-LParam $from[0] $from[1]
    Post-Burst @(0x0200, 0x0201) @(0, 1) @($fromLParam, $fromLParam)   # [WM_MOUSEMOVE · WM_LBUTTONDOWN]
    Start-Sleep -Milliseconds 300
    for ($s = 1; $s -le 4; $s++) {
        $x = [int]($from[0] + ($to[0] - $from[0]) * $s / 5)
        $y = [int]($from[1] + ($to[1] - $from[1]) * $s / 5)
        Post-Mouse $hwnd 0x0200 1 $x $y           # WM_MOUSEMOVE (왼쪽 누름)
        Start-Sleep -Milliseconds 100
    }
    $toLParam = Make-LParam $to[0] $to[1]
    Post-Burst @(0x0200, 0x0202) @(1, 0) @($toLParam, $toLParam)   # [WM_MOUSEMOVE · WM_LBUTTONUP]
    Start-Sleep -Milliseconds 800
}

# 앱 프레임 간격(묶음 넣기 전 맞추기의 기준). 첫 호출의 준비 비용도 여기서 치른다.
$frameWaits = @()
for ($k = 0; $k -lt 10; $k++) { $frameWaits += [GUIE2E]::SyncFrame($hwnd) }
$frameWaits = $frameWaits | Sort-Object
Write-Output ("Frame sync (WM_NULL): min {0:N1} ms, median {1:N1} ms, max {2:N1} ms" -f $frameWaits[0], $frameWaits[5], $frameWaits[9])

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

# 끌어다 놓기(GG-15). 금색 카드 가운데 (330, 914), 놓을 곳 (700~1120, 789~1040) 안 카드 자리 가운데 (1010, 914).
# 뷰포트 이미지 밖 = 기준 x 2400(Scene Viewport 창 오른쪽 밖, 런처 창 안) — 누른 채 나가도 이동 · 뗌이 게임 UI 로 간다.
Drag-Ref "Step 21: drag the gold card onto the drop zone" 330 914 1010 914
Drag-Ref "Step 22: drag it again and release outside the viewport image (no drop target)" 1010 914 2400 914

# 열림 · 닫힘 전환(GG-16). 일시정지 메뉴: 열림 0.3초(흐려지며 + 패널이 올라옴), 닫힘 0.4초(흐려지며 + 패널이 내려감).
# 닫히는 메뉴는 스택에서 바로 빠진다 — 입력 · 뒤로가기를 받지 않고, 같은 객체를 다시 Push 하면 지금 모습에서 다시 열린다(HUD Menu 버튼은 한 객체를 다시 올린다).
Click-Ref "Step 23: Menu (open transition)" 1770 48
Start-Sleep -Seconds 1
Write-Output "Step 24: Esc, then right away a click where Resume is (the closing menu must not take it: it goes to the world)"
Post-EscapeQuick $hwnd
Click-RefQuick "Step 24: click on Resume while the menu closes" 960 640
Start-Sleep -Milliseconds 250
$closingShot = Capture $hwnd (Join-Path $OutDir "gameui_e2e_closing.png")   # 닫힘 0.4초의 가운데쯤(흐려지며 패널이 내려가는 중)
$closingShot.Dispose()
Start-Sleep -Seconds 1
Click-RefQuick "Step 25: Menu (open again)" 1770 48
Start-Sleep -Milliseconds 150
$openingShot = Capture $hwnd (Join-Path $OutDir "gameui_e2e_opening.png")   # 열림 0.3초의 가운데쯤(흐려진 채 패널이 올라오는 중)
$openingShot.Dispose()
Start-Sleep -Seconds 1
Write-Output "Step 26: Esc, then right away Menu (reopen while closing: no OnShutdown, no OnInitialize)"
Post-EscapeQuick $hwnd
Click-RefQuick "Step 26: Menu while the menu closes" 1770 48
Start-Sleep -Seconds 1
Click-Ref "Step 27: Resume (close)" 960 640
Start-Sleep -Seconds 1

# 스크롤 칸(GG-20). 목록 칸 450~680 × 789~1040(줄 40개 · 간격 40 · 폭 320 → 보이는 칸 214 x 235, 넘길 수 있는 양 106 x 1361, 바로 넘김):
#   세로 막대 666~678 × 791~1022(손잡이 34), 가로 막대 452~662 × 1026~1038(손잡이 140). 이야기 칸 680~1220 × 640~780(20줄 · 줄 간격 22, 부드럽게 0.15초):
#   세로 막대 1206~1218 × 642~778. 손잡이 자리는 넘긴 양에 따라 바뀐다 — 아래 좌표는 앞 단계 결과를 따른 것(README 기대 결과).
Wheel-Ref "Step 28: wheel down 3 notches over the list" 565 900 -3
Click-Ref "Step 29: click a list row (offset 180 -> row 5)" 520 820
Click-Ref "Step 30: click below the list where a cut row would be (world)" 565 1060
Drag-Ref "Step 31: drag the vertical thumb past the bottom" 672 834 672 1075
Click-Ref "Step 32: press the vertical track above the thumb (page up)" 672 800
Wheel-Ref "Step 33: wheel over the top bar (no box: consumed, nothing scrolls)" 480 48 -1
Drag-Ref "Step 34: drag the horizontal thumb past the right end" 522 1032 760 1032
Wheel-Ref "Step 35: horizontal wheel left over the list" 565 900 -1 0x020E
Wheel-Ref "Step 36: wheel down 2 notches over the story (smooth)" 950 710 -2
Start-Sleep -Milliseconds 500
Click-Ref "Step 37: press the story track below the thumb (page down, smooth)" 1212 770
Start-Sleep -Seconds 1
$scrollShot = Capture $hwnd (Join-Path $OutDir "gameui_e2e_scroll.png")
$scrollShot.Dispose()

# 휠 받기(GG-21). 지도 틀 1240~1880 × 430~580(자식 잘라내기 · 끌기 · 휠 받기), 안의 지도 1280 × 300 을 배율 0.5 로 시작(틀에 꼭 맞음).
# 휠 한 칸 = 1.15배(포인터 기준, 0.12초 OutCubic 트윈). 끝나면 "map zoom done … moved 0.00"(포인터 아래 점이 그대로).
Wheel-Ref "Step 38: wheel forward 2 notches over the map (zoom in around the pointer)" 1400 505 2
Wheel-Ref "Step 39: wheel back 1 notch at another place (zoom out)" 1700 470 -1
Drag-Ref "Step 40: drag the map frame (pan the map)" 1500 520 1380 480
DragWheel-Ref "Step 41: wheel while dragging the map (the dragged element gets the wheel)" 1600 500 1560 500 1540 500 1
Start-Sleep -Seconds 1
$mapShot = Capture $hwnd (Join-Path $OutDir "gameui_e2e_map.png")
$mapShot.Dispose()

$after = Capture $hwnd (Join-Path $OutDir "gameui_e2e_after.png")
$after.Dispose()

[GUIE2E]::PostMessage($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null   # WM_CLOSE
if (-not $proc.WaitForExit(30000)) { Write-Output "Timeout waiting exit, killing"; $proc.Kill(); exit 5 }
Write-Output "ExitCode $($proc.ExitCode)"

$waits = $script:BurstWaits | Where-Object { $_ -ge 0 } | Sort-Object
$timeouts = ($script:BurstWaits | Where-Object { $_ -lt 0 }).Count
if ($waits.Count -gt 0) {
    Write-Output ("Input bursts {0}: frame sync wait median {1:N1} ms, max {2:N1} ms, timeouts {3}" -f $script:BurstWaits.Count, $waits[[int]($waits.Count / 2)], $waits[$waits.Count - 1], $timeouts)
}
if ($NoCheck) {
    exit 0
}
exit (Invoke-E2ECheck (Read-LogFile (Join-Path $BinDir "jg_log.txt")) $proc.ExitCode)
