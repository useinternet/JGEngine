param(
    [int]$WaitSeconds = 20,
    [string]$OutPath = "",
    [string]$WorkDir = ""
)
# 런처(JGLauncher)를 띄우고 → 메인 창이 뜬 뒤 WaitSeconds 초 기다려 → PrintWindow 로 창을 캡처하고 → WM_CLOSE 로 닫아 종료 코드를 확인하는 공용 도구.
# 경로는 이 스크립트 위치(<저장소>\Tools\Common) 기준으로 정한다 — 저장소 폴더가 어디에 있어도 그대로 돈다.
# (2026-10-11 Graphics: 옛 PC 경로 C:\JG\JGEngine 하드코딩을 없앰)
#   -WaitSeconds  창이 뜬 뒤 캡처까지 기다리는 초(기본 20)
#   -OutPath      캡처 PNG 경로(기본 <저장소>\Temp\Capture\devscene_capture.png — git 이 무시하는 폴더, 매번 덮어씀). 폴더가 없으면 만든다
#   -WorkDir      런처 작업 폴더(기본 <저장소>\Bin\DevelopEngine). imgui.ini · jg_log.txt · DevScene 리드백 PNG 는 이 폴더 것을 쓴다.
#                 사용자 창 배치(메인 imgui.ini)를 건드리지 않고 DevScene 을 열려면 [JGWidget][Open] 아래 JGDevFeature=1 이 든 imgui.ini 를 둔 다른 폴더를 준다
# 메인 Bin 을 쓰므로 다른 세션의 빌드와 겹치지 않게 감싼다(지침서 3절):
#   Build\BatchFiles\MainTree.cmd -name=<세션> exec Tools\Common\capture_devscene.ps1 [-WaitSeconds 30] [-OutPath x.png] [-WorkDir 폴더]
# 종료 코드: 0 정상, 1 런처 · 작업 폴더 없음, 2 창이 뜨기 전에 끝남, 3 창 없음, 4 기다리는 동안 끝남, 5 닫기 시간 초과

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32Capture {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdcBlt, uint nFlags);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
}
"@

# <저장소> = Common → Tools 의 위
$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\.."))
$exe = Join-Path $repoRoot "Bin\DevelopEngine\JGLauncher.exe"
if ($WorkDir -eq "")
{
    $WorkDir = Join-Path $repoRoot "Bin\DevelopEngine"
}
if ($OutPath -eq "")
{
    $OutPath = Join-Path $repoRoot "Temp\Capture\devscene_capture.png"
}
# 상대 경로는 지금 PowerShell 위치 기준으로 푼다(Bitmap.Save 는 .NET 의 현재 폴더를 보므로 다를 수 있다)
$WorkDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($WorkDir)
$OutPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutPath)

if (-not (Test-Path -LiteralPath $exe -PathType Leaf))
{
    Write-Output "Launcher not found: $exe"
    exit 1
}
if (-not (Test-Path -LiteralPath $WorkDir -PathType Container))
{
    Write-Output "Work folder not found: $WorkDir"
    exit 1
}
$outDir = Split-Path -Parent $OutPath
if (-not (Test-Path -LiteralPath $outDir -PathType Container))
{
    New-Item -ItemType Directory -Force -Path $outDir | Out-Null
}
Write-Output "Launcher $exe"
Write-Output "WorkDir  $WorkDir"

$proc = Start-Process -FilePath $exe -WorkingDirectory $WorkDir -PassThru
Write-Output "Started PID $($proc.Id)"

# 메인 창이 뜰 때까지 기다린다
$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 60; $i++) {
    Start-Sleep -Milliseconds 500
    $proc.Refresh()
    if ($proc.HasExited) { Write-Output "Process exited early with code $($proc.ExitCode)"; exit 2 }
    if ($proc.MainWindowHandle -ne [IntPtr]::Zero) { $hwnd = $proc.MainWindowHandle; break }
}
if ($hwnd -eq [IntPtr]::Zero) { Write-Output "No main window"; $proc.Kill(); exit 3 }
Write-Output "Main window found: $hwnd"

Start-Sleep -Seconds $WaitSeconds
$proc.Refresh()
if ($proc.HasExited) { Write-Output "Process exited during wait with code $($proc.ExitCode)"; exit 4 }

[Win32Capture]::SetForegroundWindow($hwnd) | Out-Null
Start-Sleep -Milliseconds 500

$rect = New-Object Win32Capture+RECT
[Win32Capture]::GetWindowRect($hwnd, [ref]$rect) | Out-Null
$width  = $rect.Right - $rect.Left
$height = $rect.Bottom - $rect.Top
Write-Output "Window rect: $($rect.Left),$($rect.Top) ${width}x${height}"

$bmp = New-Object System.Drawing.Bitmap($width, $height)
$gfx = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $gfx.GetHdc()
# PW_RENDERFULLCONTENT = 2 : DirectX 스왑체인 창도 캡처된다
$ok = [Win32Capture]::PrintWindow($hwnd, $hdc, 2)
$gfx.ReleaseHdc($hdc)
if (-not $ok) {
    Write-Output "PrintWindow failed, fallback to CopyFromScreen"
    $gfx.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bmp.Size)
}
$gfx.Dispose()
$bmp.Save($OutPath, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Output "Saved $OutPath"

# WM_CLOSE 로 정상 종료
[Win32Capture]::PostMessage($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
if (-not $proc.WaitForExit(30000)) { Write-Output "Timeout waiting exit, killing"; $proc.Kill(); exit 5 }
Write-Output "ExitCode $($proc.ExitCode)"
exit 0
