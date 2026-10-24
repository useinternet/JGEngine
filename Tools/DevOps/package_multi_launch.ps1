# Package multi-launch log check (DevOps ER-024, 2026-10-11).
# Starts the same packaged <Name>.exe N times at once (-smoke=<Frames>, small windowed window), waits for all of them,
# then checks Saved/Logs: every run must have its own log file with exactly one "Package ..." start line, one
# "Log file ... (process <pid>)" line naming that run and one "GameHost: shutdown" line, and no log may contain lines
# of another run (spdlog lines carry the thread id, and the thread id sets of the files must not overlap).
# usage: powershell -NoProfile -ExecutionPolicy Bypass -File package_multi_launch.ps1 -Package <package folder> [-Count 3]
#        [-Frames 300] [-Arguments "-logfile=Host.log"]
param(
    [Parameter(Mandatory = $true)][string]$Package,
    [int]$Count = 3,
    [int]$Frames = 300,
    [string]$Arguments = ""
)

$Package = (Resolve-Path -LiteralPath $Package).ProviderPath
$exe = Get-ChildItem -LiteralPath $Package -Filter *.exe | Select-Object -First 1
if ($null -eq $exe) { Write-Output "No exe in $Package"; exit 1 }
$logs = Join-Path $Package "Saved\Logs"
$started = Get-Date

$processes = @()
for ($i = 0; $i -lt $Count; $i++) {
    $argumentList = "-smoke=$Frames -windowed -res=640x360 $Arguments".Trim()
    $processes += Start-Process -FilePath $exe.FullName -ArgumentList $argumentList -WorkingDirectory $Package -PassThru
}
foreach ($process in $processes) { $null = $process.Handle }

$failed = $false
foreach ($process in $processes) {
    if (-not $process.WaitForExit(120000)) { Write-Output ("pid {0} did not exit in 120 s" -f $process.Id); $process.Kill(); $failed = $true; continue }
    Write-Output ("pid {0} exit {1}" -f $process.Id, $process.ExitCode)
    if ($process.ExitCode -ne 0) { $failed = $true }
}

$pids = $processes | ForEach-Object { $_.Id }
$touched = Get-ChildItem -LiteralPath $logs -File | Where-Object { $_.LastWriteTime -ge $started.AddSeconds(-1) } | Sort-Object Name
Write-Output ("log files written by this run: {0}" -f (($touched | ForEach-Object { $_.Name }) -join ", "))
if ($touched.Count -ne $Count) { Write-Output ("expected {0} log files" -f $Count); $failed = $true }

$threadSets = @{}
$seenPids = @{}
foreach ($file in $touched) {
    $lines = Get-Content -LiteralPath $file.FullName
    $startLines = @($lines | Where-Object { $_ -match '\]: Package ' }).Count
    $endLines = @($lines | Where-Object { $_ -match 'GameHost: shutdown' }).Count
    $errorLines = @($lines | Where-Object { $_ -match '\[(error|critical)\]' }).Count
    $logLine = @($lines | Where-Object { $_ -match 'Log file .* \(process (\d+)\)' })
    $logPid = ""
    if ($logLine.Count -eq 1 -and $logLine[0] -match '\(process (\d+)\)') { $logPid = $Matches[1] }
    $threads = @($lines | ForEach-Object { if ($_ -match '^\[:(\d+):\]') { $Matches[1] } } | Sort-Object -Unique)
    $threadSets[$file.Name] = $threads
    Write-Output ("{0}: {1} lines, start {2}, shutdown {3}, error {4}, process {5}, threads {6}" -f $file.Name, $lines.Count, $startLines, $endLines, $errorLines, $logPid, $threads.Count)
    if ($startLines -ne 1 -or $endLines -ne 1 -or $errorLines -ne 0 -or $logPid -eq "") { $failed = $true }
    if ($logPid -ne "") {
        if ($pids -notcontains [int]$logPid) { Write-Output "  process $logPid is not one of this run"; $failed = $true }
        if ($seenPids.ContainsKey($logPid)) { Write-Output "  process $logPid also wrote $($seenPids[$logPid])"; $failed = $true }
        $seenPids[$logPid] = $file.Name
    }
    $moved = @($lines | Where-Object { $_ -match 'is in use by another run of this package' })
    if ($moved.Count -gt 0) { Write-Output ("  " + ($moved[0] -replace '^\[:\d+:\]\[info\]', '')) }
}

$names = @($threadSets.Keys)
for ($a = 0; $a -lt $names.Count; $a++) {
    for ($b = $a + 1; $b -lt $names.Count; $b++) {
        $shared = @($threadSets[$names[$a]] | Where-Object { $threadSets[$names[$b]] -contains $_ })
        if ($shared.Count -gt 0) { Write-Output ("{0} and {1} share thread ids {2} - lines are mixed" -f $names[$a], $names[$b], ($shared -join ",")); $failed = $true }
    }
}

if ($failed) { Write-Output "RESULT: FAIL"; exit 1 }
Write-Output "RESULT: OK"
exit 0
