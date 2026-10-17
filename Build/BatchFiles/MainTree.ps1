# JGEngine main tree guard (DevOps, 2026-10-03).
# Several sessions build and run the same engine tree. A link while another session's editor holds the DLLs
# (LNK1104 / LNK1168), or an editor started while another build relinks a DLL ("Fail Connect Module"), breaks
# both. This script waits until the tree is free, takes a lock, runs the build or the command, and releases it.
# Start it through MainTree.cmd, which hands over its raw arguments in JG_MAINTREE_ARGS:
#
#   MainTree.cmd -name=WHO status
#   MainTree.cmd -name=WHO [-wait=MINUTES] build [-config=CFG] [-project=NAME_OR_PATH] [-target=T] [MSBuild args]
#   MainTree.cmd -name=WHO [-wait=MINUTES] exec PROGRAM [args]
#   MainTree.cmd -name=WHO [-wait=MINUTES] hold [-minutes=N]
#   MainTree.cmd -name=WHO release
#   MainTree.cmd -name=WHO [-wait=SECONDS] editor [-project=GAME_PROJECT_DIR]      (RunEditor.cmd calls this)
#
# build   MSBuild (Visual Studio 2022) on JGEngine.sln, DevelopEngine x64 by default. -project= takes a project name
#         (Temp\ProjectFiles\NAME.vcxproj) or a path relative to the tree. Arguments after the options go to MSBuild.
# exec    runs any program, .cmd/.bat or .ps1 in the current directory and returns its exit code.
# hold    keeps the lock for a sequence of steps (copy into the tree, generate, build, run) for up to -minutes
#         (default 15, at most 60). build and exec with the same -name= run inside the hold; release ends it.
# status  shows who uses the tree. Exit code 0 free, 1 busy.
# editor  starts the editor (Bin\DevelopEngine\JGLauncher.exe, working in that folder) on its own and returns at
#         once - for the dashboard button. It does not start while the tree is busy with a build (lock, outside
#         builds, recent build output; running Bin\ programs do not count), waiting -wait=SECONDS (default 0)
#         first, and never starts a second editor of the same Bin. It holds the lock only while it starts the
#         editor; builds then wait for the running editor as usual. -project= starts the editor of a game project
#         (PROJECT\Bin\DevelopEngine) and looks only at that project's builds. One line of output.
#         Exit codes: 0 started, 89 already running, 90 busy, 91 usage, 92 launcher or project not found,
#         93 the editor did not start or ended at once.
#
# The tree is busy when:
#   - the lock Temp\MainTree\lock is taken by another build, exec or hold;
#   - an MSBuild was started on this tree's solution or projects, or a cl/link/lib/rc/mt/cvtres/premake5 process
#     works in a folder of this tree (command line, Visual Studio and Rider builds alike);
#   - build output (Bin\CONFIG\*.dll *.exe *.lib *.exp *.pdb *.ilk *.idb, anything under Temp\Obj) was written in
#     the last -quiet seconds (default 10);
#   - build and hold only: a process runs from this tree's Bin\ (editor, launcher, console) and holds the DLLs.
#     exec does not wait for those; it only notes them (they share Bin\CONFIG\jg_log.txt).
# Worktrees and test games under Temp\wt\ and Temp\Games\ are other trees and never count.
# -wait=MINUTES (default 6) is how long to wait; -wait=0 fails at once when busy.
# Exit codes: the build's or command's own code; 90 still busy after -wait; 91 usage error; 92 MSBuild or program
# not found. release: 0 released, 1 no hold of yours. History: Temp\MainTree\history.log.

$ErrorActionPreference = "Stop"

$ExitTimedOut = 90
$ExitUsage = 91
$ExitNotFound = 92
$ExitEditorRunning = 89
$ExitEditorFailed = 93
$Invariant = [Globalization.CultureInfo]::InvariantCulture
$PowerShellPath = [IO.Path]::Combine($PSHOME, "powershell.exe")
$BuildToolNames = @("msbuild.exe", "cl.exe", "link.exe", "lib.exe", "rc.exe", "mt.exe", "cvtres.exe", "premake5.exe")
$BuildOutputExtensions = @(".dll", ".exe", ".lib", ".exp", ".pdb", ".ilk", ".idb")

# Sessions read this output through pipes that expect UTF-8; a console gets it through the console API instead.
if ([Console]::IsOutputRedirected)
{
    $script:Out = New-Object IO.StreamWriter([Console]::OpenStandardOutput(), (New-Object Text.UTF8Encoding($false)))
    $script:Out.AutoFlush = $true
}
else
{
    $script:Out = [Console]::Out
}

function Write-Guard([string]$text)
{
    $script:Out.WriteLine("[MainTree] " + $text)
}

Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class JGMainTreeNative
{
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern IntPtr OpenProcess(uint access, bool inheritHandle, int processId);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool CloseHandle(IntPtr handle);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool ReadProcessMemory(IntPtr process, IntPtr address, byte[] buffer, IntPtr size, out IntPtr read);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool IsWow64Process(IntPtr process, out bool wow64);

    [DllImport("ntdll.dll")]
    static extern int NtQueryInformationProcess(IntPtr process, int infoClass, byte[] info, int length, out int returned);

    const uint ProcessQueryInformation = 0x0400;
    const uint ProcessVmRead = 0x0010;

    static byte[] Read(IntPtr process, long address, int size)
    {
        byte[] buffer = new byte[size];
        IntPtr read;
        if (ReadProcessMemory(process, new IntPtr(address), buffer, new IntPtr(size), out read) == false)
        {
            return null;
        }
        return buffer;
    }

    // Working directory of another process of this user, read from its process parameters (64-bit and WOW64
    // processes). null when it cannot be read.
    public static string GetCurrentDirectory(int processId)
    {
        if (IntPtr.Size != 8)
        {
            return null;
        }
        IntPtr process = OpenProcess(ProcessQueryInformation | ProcessVmRead, false, processId);
        if (process == IntPtr.Zero)
        {
            return null;
        }
        try
        {
            bool wow64;
            if (IsWow64Process(process, out wow64) == false)
            {
                return null;
            }
            int returned;
            long parameters;
            byte[] directory;
            int length;
            long buffer;
            if (wow64)
            {
                // ProcessWow64Information: address of the 32-bit PEB. PEB32.ProcessParameters at 0x10,
                // RTL_USER_PROCESS_PARAMETERS32.CurrentDirectory.DosPath at 0x24 (Length, MaximumLength, Buffer32).
                byte[] information = new byte[8];
                if (NtQueryInformationProcess(process, 26, information, information.Length, out returned) != 0)
                {
                    return null;
                }
                byte[] peb = Read(process, BitConverter.ToInt64(information, 0) + 0x10, 4);
                if (peb == null)
                {
                    return null;
                }
                parameters = BitConverter.ToUInt32(peb, 0);
                directory = Read(process, parameters + 0x24, 8);
                if (directory == null)
                {
                    return null;
                }
                length = BitConverter.ToUInt16(directory, 0);
                buffer = BitConverter.ToUInt32(directory, 4);
            }
            else
            {
                // ProcessBasicInformation: PebBaseAddress at 8. PEB.ProcessParameters at 0x20,
                // RTL_USER_PROCESS_PARAMETERS.CurrentDirectory.DosPath at 0x38 (Length, MaximumLength, Buffer at 8).
                byte[] information = new byte[48];
                if (NtQueryInformationProcess(process, 0, information, information.Length, out returned) != 0)
                {
                    return null;
                }
                byte[] peb = Read(process, BitConverter.ToInt64(information, 8) + 0x20, 8);
                if (peb == null)
                {
                    return null;
                }
                parameters = BitConverter.ToInt64(peb, 0);
                directory = Read(process, parameters + 0x38, 16);
                if (directory == null)
                {
                    return null;
                }
                length = BitConverter.ToUInt16(directory, 0);
                buffer = BitConverter.ToInt64(directory, 8);
            }
            if (length <= 0 || length > 4096)
            {
                return null;
            }
            byte[] text = Read(process, buffer, length);
            if (text == null)
            {
                return null;
            }
            return Encoding.Unicode.GetString(text);
        }
        catch (Exception)
        {
            return null;
        }
        finally
        {
            CloseHandle(process);
        }
    }
}
"@

function Format-Time([DateTime]$time)
{
    return $time.ToString("yyyy-MM-dd HH:mm:ss", $Invariant)
}

function Format-Span([TimeSpan]$span)
{
    $seconds = [int][Math]::Floor([Math]::Max(0, $span.TotalSeconds))
    if ($seconds -lt 60)
    {
        return ("{0}s" -f $seconds)
    }
    return ("{0}m{1:00}s" -f [int][Math]::Floor($seconds / 60), ($seconds % 60))
}

# Splits a command line into tokens. Double quotes group text and are dropped from Value; Start is where the
# token begins, so the rest of a command line can be passed on untouched.
function Split-CommandLine([string]$text)
{
    $tokens = New-Object Collections.Generic.List[object]
    $index = 0
    while ($index -lt $text.Length)
    {
        while ($index -lt $text.Length -and [char]::IsWhiteSpace($text[$index]))
        {
            $index++
        }
        if ($index -ge $text.Length)
        {
            break
        }
        $start = $index
        $value = New-Object Text.StringBuilder
        $quoted = $false
        while ($index -lt $text.Length -and ($quoted -or -not [char]::IsWhiteSpace($text[$index])))
        {
            if ($text[$index] -eq '"')
            {
                $quoted = -not $quoted
            }
            else
            {
                [void]$value.Append($text[$index])
            }
            $index++
        }
        $tokens.Add([pscustomobject]@{ Value = $value.ToString(); Start = $start })
    }
    return ,$tokens
}

function Get-Shortened([string]$text, [int]$length)
{
    $text = $text.Trim()
    if ($text.Length -le $length)
    {
        return $text
    }
    return $text.Substring(0, $length) + "..."
}

# ---- arguments ----

$rawArguments = [string]$env:JG_MAINTREE_ARGS
Remove-Item Env:JG_MAINTREE_ARGS -ErrorAction SilentlyContinue
$tokens = Split-CommandLine $rawArguments
$options = @{ name = ""; wait = "6"; root = ""; quiet = "10"; config = "DevelopEngine"; project = ""; target = "Build"; minutes = "15"; token = ""; parent = "0" }
$given = @{}
$verb = ""
$rest = ""
$tokenIndex = 0
while ($tokenIndex -lt $tokens.Count)
{
    $value = $tokens[$tokenIndex].Value
    if ($value -match '^-(name|wait|root|quiet|config|project|target|minutes|token|parent)=(.*)$')
    {
        $options[$Matches[1].ToLowerInvariant()] = $Matches[2]
        $given[$Matches[1].ToLowerInvariant()] = $true
        $tokenIndex++
        continue
    }
    if ($verb -eq "")
    {
        $verb = $value.ToLowerInvariant()
        $tokenIndex++
        continue
    }
    break
}
if ($tokenIndex -lt $tokens.Count)
{
    $rest = $rawArguments.Substring($tokens[$tokenIndex].Start).Trim()
}

function Exit-Usage([string]$message)
{
    if ($verb -eq "editor")
    {
        $script:Out.WriteLine("사용법이 틀림 — " + $message + " (RunEditor.cmd [-wait=초] [-name=누구] [-project=게임 프로젝트 폴더])")
        exit $ExitUsage
    }
    if ($message -ne "")
    {
        Write-Guard $message
    }
    Write-Guard "usage: MainTree.cmd -name=WHO [-wait=MINUTES] status | build [-config=CFG] [-project=NAME_OR_PATH] [-target=T] [MSBuild args] | exec PROGRAM [args] | hold [-minutes=N] | release | editor (-wait= in seconds) [-project=GAME_PROJECT_DIR]"
    exit $ExitUsage
}

function Get-IntegerOption([string]$key, [int]$minimum, [int]$maximum)
{
    $number = 0
    if (-not [int]::TryParse($options[$key], [ref]$number) -or $number -lt $minimum -or $number -gt $maximum)
    {
        Exit-Usage ("-{0}= must be a number from {1} to {2}" -f $key, $minimum, $maximum)
    }
    return $number
}

$name = $options.name.Trim()
if ($name -eq "")
{
    $name = "unnamed"
}
$waitMinutes = Get-IntegerOption "wait" 0 600
$quietSeconds = Get-IntegerOption "quiet" 0 300
$holdMinutes = Get-IntegerOption "minutes" 1 60
$currentDirectory = (Get-Location).ProviderPath

if ($options.root -ne "")
{
    $root = [IO.Path]::GetFullPath($options.root)
}
else
{
    $root = [IO.Path]::GetFullPath([IO.Path]::Combine($PSScriptRoot, "..\.."))
}
$root = $root.TrimEnd('\') + '\'
$guardDirectory = $root + "Temp\MainTree"
$lockPath = $guardDirectory + "\lock"
$ownerPath = $guardDirectory + "\owner.txt"
$historyPath = $guardDirectory + "\history.log"
$otherTrees = @(($root + "Temp\wt\"), ($root + "Temp\Games\"))
[void][IO.Directory]::CreateDirectory($guardDirectory)

# ---- small files: owner, waiters, history ----

function Read-KeyValueFile([string]$path)
{
    for ($attempt = 0; $attempt -lt 3; $attempt++)
    {
        try
        {
            if (-not [IO.File]::Exists($path))
            {
                return $null
            }
            $stream = New-Object IO.FileStream($path, [IO.FileMode]::Open, [IO.FileAccess]::Read, ([IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete))
            $reader = New-Object IO.StreamReader($stream, [Text.Encoding]::UTF8)
            $text = $reader.ReadToEnd()
            $reader.Dispose()
            $values = @{}
            foreach ($line in ($text -split "`r?`n"))
            {
                $at = $line.IndexOf('=')
                if ($at -gt 0)
                {
                    $values[$line.Substring(0, $at)] = $line.Substring($at + 1)
                }
            }
            return $values
        }
        catch
        {
            Start-Sleep -Milliseconds 100
        }
    }
    return $null
}

function Write-KeyValueFile([string]$path, [hashtable]$values)
{
    $lines = foreach ($key in $values.Keys)
    {
        $key + "=" + $values[$key]
    }
    [IO.File]::WriteAllText($path, (($lines -join "`r`n") + "`r`n"), (New-Object Text.UTF8Encoding($false)))
}

function Add-History([string]$text)
{
    $line = (Format-Time (Get-Date)) + " | " + $name + " | " + $text + "`r`n"
    $bytes = (New-Object Text.UTF8Encoding($false)).GetBytes($line)
    for ($attempt = 0; $attempt -lt 10; $attempt++)
    {
        try
        {
            $stream = New-Object IO.FileStream($historyPath, [IO.FileMode]::Append, [IO.FileAccess]::Write, [IO.FileShare]::ReadWrite)
            $stream.Write($bytes, 0, $bytes.Length)
            $stream.Dispose()
            return
        }
        catch
        {
            Start-Sleep -Milliseconds 50
        }
    }
}

function Get-TreeRelative([string]$path)
{
    if ($path.StartsWith($root, [StringComparison]::OrdinalIgnoreCase))
    {
        return $path.Substring($root.Length)
    }
    return $path
}

function Parse-Time([string]$text)
{
    $time = [DateTime]::MinValue
    if ([DateTime]::TryParseExact($text, "yyyy-MM-dd HH:mm:ss", $Invariant, [Globalization.DateTimeStyles]::None, [ref]$time))
    {
        return $time
    }
    return $null
}

# ---- the lock: an exclusive handle on Temp\MainTree\lock; it goes away with the process that holds it ----

$script:LockStream = $null

function Enter-TreeLock
{
    try
    {
        $script:LockStream = [IO.File]::Open($lockPath, [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
        return $true
    }
    catch
    {
        $script:LockStream = $null
        return $false
    }
}

function Close-TreeLock
{
    if ($null -ne $script:LockStream)
    {
        $script:LockStream.Dispose()
        $script:LockStream = $null
    }
}

function Exit-TreeLock
{
    if ($null -ne $script:LockStream)
    {
        try
        {
            [IO.File]::Delete($ownerPath)
        }
        catch
        {
        }
        Close-TreeLock
    }
}

function Test-TreeLockFree
{
    if (Enter-TreeLock)
    {
        Close-TreeLock
        return $true
    }
    return $false
}

function Get-LockHolderText
{
    $owner = Read-KeyValueFile $ownerPath
    if ($null -eq $owner -or -not $owner.ContainsKey("name"))
    {
        return "lock: taken (owner not written yet)"
    }
    $text = "lock: {0} - {1}" -f $owner["name"], $owner["action"]
    $since = Parse-Time ([string]$owner["since"])
    if ($null -ne $since)
    {
        $text += ", for " + (Format-Span ((Get-Date) - $since))
    }
    if ($owner.ContainsKey("until") -and $owner["until"] -ne "")
    {
        $text += ", held until " + $owner["until"].Substring(11)
    }
    return $text + ", pid " + $owner["pid"]
}

# A hold taken earlier by this -name= that is still in force: build and exec then run inside it.
function Test-OwnHold
{
    if ($name -eq "unnamed")
    {
        return $false
    }
    $owner = Read-KeyValueFile $ownerPath
    if ($null -eq $owner -or $owner["kind"] -ne "hold" -or $owner["name"] -ne $name)
    {
        return $false
    }
    return -not (Test-TreeLockFree)
}

# ---- activity in the tree that does not go through this script ----

function Test-InTree([string]$path)
{
    if ([string]::IsNullOrEmpty($path))
    {
        return $false
    }
    try
    {
        $full = [IO.Path]::GetFullPath($path).TrimEnd('\') + '\'
    }
    catch
    {
        return $false
    }
    if (-not $full.StartsWith($root, [StringComparison]::OrdinalIgnoreCase))
    {
        return $false
    }
    foreach ($other in $otherTrees)
    {
        if ($full.StartsWith($other, [StringComparison]::OrdinalIgnoreCase))
        {
            return $false
        }
    }
    return $true
}

# What an MSBuild command line builds: the first argument that is not a switch or a response file. A relative
# one cannot be resolved later - MSBuild moves its working directory into each project it builds - so the
# working directory decides then. Without an argument MSBuild builds the project in the working directory.
function Get-MSBuildTarget([string]$commandLine, [string]$workingDirectory)
{
    $arguments = Split-CommandLine $commandLine
    for ($index = 1; $index -lt $arguments.Count; $index++)
    {
        $value = $arguments[$index].Value
        if ($value.StartsWith("-") -or $value.StartsWith("/") -or $value.StartsWith("@"))
        {
            continue
        }
        if ([IO.Path]::IsPathRooted($value))
        {
            return [pscustomobject]@{ Path = $value; Text = $value }
        }
        return [pscustomobject]@{ Path = $workingDirectory; Text = ("{0}, working in {1}" -f $value, $workingDirectory) }
    }
    return [pscustomobject]@{ Path = $workingDirectory; Text = $workingDirectory }
}

function Get-TreeActivity
{
    $builds = New-Object Collections.Generic.List[string]
    $apps = New-Object Collections.Generic.List[string]
    $ides = New-Object Collections.Generic.List[string]
    $tools = @{}
    $binPrefix = $root + "Bin\"
    try
    {
        $processes = Get-CimInstance -ClassName Win32_Process -Property ProcessId, Name, ExecutablePath, CommandLine, CreationDate
    }
    catch
    {
        $builds.Add("process list not available (" + $_.Exception.Message + ")")
        return [pscustomobject]@{ Builds = $builds; Apps = $apps; Ides = $ides }
    }
    foreach ($process in $processes)
    {
        $processId = [int]$process.ProcessId
        if ($processId -eq $PID)
        {
            continue
        }
        $processName = ([string]$process.Name).ToLowerInvariant()
        $commandLine = [string]$process.CommandLine
        if ($BuildToolNames -contains $processName)
        {
            $workingDirectory = [JGMainTreeNative]::GetCurrentDirectory($processId)
            if ($processName -eq "msbuild.exe")
            {
                # Worker nodes carry no project; their compiles and links show up as cl / link below.
                if ($commandLine -match '[-/]nodemode[:=]')
                {
                    continue
                }
                $target = Get-MSBuildTarget $commandLine $workingDirectory
                if (Test-InTree $target.Path)
                {
                    $builds.Add(("MSBuild pid {0} ({1})" -f $processId, $target.Text))
                }
            }
            elseif (Test-InTree $workingDirectory)
            {
                $tools[$processName] = 1 + [int]$tools[$processName]
            }
            continue
        }
        $path = [string]$process.ExecutablePath
        if ($path -ne "" -and $path.StartsWith($binPrefix, [StringComparison]::OrdinalIgnoreCase))
        {
            $since = ""
            if ($null -ne $process.CreationDate)
            {
                $since = " since " + $process.CreationDate.ToString("HH:mm:ss", $Invariant)
            }
            $arguments = ""
            $parts = Split-CommandLine $commandLine
            if ($parts.Count -gt 1)
            {
                $arguments = " " + (Get-Shortened $commandLine.Substring($parts[1].Start) 80)
            }
            $apps.Add(("{0}{1} (pid {2}{3})" -f $path.Substring($binPrefix.Length), $arguments, $processId, $since))
            continue
        }
        if ($processName -eq "devenv.exe")
        {
            $ides.Add(("Visual Studio pid {0}" -f $processId))
        }
        elseif ($processName -eq "rider64.exe")
        {
            $ides.Add(("Rider pid {0}" -f $processId))
        }
    }
    foreach ($tool in $tools.Keys)
    {
        $builds.Add(("{0} x{1} working in this tree" -f $tool, $tools[$tool]))
    }
    return [pscustomobject]@{ Builds = $builds; Apps = $apps; Ides = $ides }
}

# Newest build output; a write within -quiet seconds means some build is still running.
function Get-RecentBuildOutput
{
    if ($quietSeconds -le 0)
    {
        return $null
    }
    $newestTime = [DateTime]::MinValue
    $newestPath = ""
    try
    {
        $bin = $root + "Bin"
        if ([IO.Directory]::Exists($bin))
        {
            foreach ($directory in (New-Object IO.DirectoryInfo($bin)).EnumerateDirectories())
            {
                foreach ($file in $directory.EnumerateFiles())
                {
                    if ($file.LastWriteTimeUtc -gt $newestTime -and $BuildOutputExtensions -contains $file.Extension.ToLowerInvariant())
                    {
                        $newestTime = $file.LastWriteTimeUtc
                        $newestPath = $file.FullName
                    }
                }
            }
        }
        $intermediate = $root + "Temp\Obj"
        if ([IO.Directory]::Exists($intermediate))
        {
            foreach ($file in (New-Object IO.DirectoryInfo($intermediate)).EnumerateFiles("*", [IO.SearchOption]::AllDirectories))
            {
                if ($file.LastWriteTimeUtc -gt $newestTime)
                {
                    $newestTime = $file.LastWriteTimeUtc
                    $newestPath = $file.FullName
                }
            }
        }
    }
    catch
    {
        return "build output is changing (" + $_.Exception.Message + ")"
    }
    $age = [DateTime]::UtcNow - $newestTime
    if ($newestPath -ne "" -and $age.TotalSeconds -lt $quietSeconds)
    {
        return ("build output written {0} ago ({1})" -f (Format-Span $age), $newestPath.Substring($root.Length))
    }
    return $null
}

# ---- waiting ----

# Waits until the tree is free and takes the lock (unless this -name= already holds it). Returns the wait, or $null
# when still busy after -wait minutes or when $parentId (the hold command) went away.
function Wait-Tree([string]$action, [bool]$withApps, [int]$parentId)
{
    $start = Get-Date
    $deadline = $start.AddMinutes($waitMinutes)
    $nextReport = $start
    $waitPath = $guardDirectory + "\wait-" + $PID + ".txt"
    $announced = $false
    try
    {
        while ($true)
        {
            $own = Test-OwnHold
            $blockers = New-Object Collections.Generic.List[string]
            $activity = $null
            if (-not $own -and -not (Enter-TreeLock))
            {
                $blockers.Add((Get-LockHolderText))
            }
            if ($blockers.Count -eq 0)
            {
                $activity = Get-TreeActivity
                foreach ($item in $activity.Builds)
                {
                    $blockers.Add($item)
                }
                if ($withApps)
                {
                    foreach ($item in $activity.Apps)
                    {
                        $blockers.Add("running Bin\" + $item)
                    }
                }
                if (-not $own)
                {
                    $recent = Get-RecentBuildOutput
                    if ($null -ne $recent)
                    {
                        $blockers.Add($recent)
                    }
                    if ($blockers.Count -gt 0)
                    {
                        # Do not sit on the lock while waiting for outside activity.
                        Close-TreeLock
                    }
                }
            }
            if ($blockers.Count -eq 0)
            {
                return [pscustomobject]@{ Waited = ((Get-Date) - $start); Own = $own; Activity = $activity }
            }
            $now = Get-Date
            if (-not $announced)
            {
                Write-KeyValueFile $waitPath @{ name = $name; action = $action; since = (Format-Time $start); pid = $PID }
                $announced = $true
            }
            if ($now -ge $nextReport)
            {
                Write-Guard ("waiting {0}: {1}" -f (Format-Span ($now - $start)), ($blockers -join "; "))
                $nextReport = $now.AddSeconds(30)
            }
            if ($now -ge $deadline)
            {
                Write-Guard ("still busy after {0} minute(s) - giving up: {1}" -f $waitMinutes, ($blockers -join "; "))
                return $null
            }
            if ($parentId -gt 0 -and $null -eq (Get-Process -Id $parentId -ErrorAction SilentlyContinue))
            {
                return $null
            }
            Start-Sleep -Milliseconds (1500 + (Get-Random -Maximum 1000))
        }
    }
    finally
    {
        if ($announced)
        {
            try
            {
                [IO.File]::Delete($waitPath)
            }
            catch
            {
            }
        }
    }
}

function Invoke-Child([string]$file, [string]$arguments, [string]$workingDirectory)
{
    $info = New-Object Diagnostics.ProcessStartInfo
    $info.FileName = $file
    $info.Arguments = $arguments
    $info.WorkingDirectory = $workingDirectory
    $info.UseShellExecute = $false
    # A MainTree call inside the command (a script that builds, say) must not wait for this lock.
    $info.EnvironmentVariables["JG_MAINTREE_HELD"] = $root
    $process = [Diagnostics.Process]::Start($info)
    $process.WaitForExit()
    $code = $process.ExitCode
    $process.Dispose()
    return $code
}

function Invoke-Guarded([string]$action, [bool]$withApps, [string]$file, [string]$arguments, [string]$workingDirectory)
{
    if ($env:JG_MAINTREE_HELD -eq $root)
    {
        Write-Guard ($action + " - inside another MainTree call, which already holds the tree")
        return (Invoke-Child $file $arguments $workingDirectory)
    }
    $wait = Wait-Tree $action $withApps 0
    if ($null -eq $wait)
    {
        Add-History ("{0} | gave up after {1}m, still busy" -f $action, $waitMinutes)
        exit $ExitTimedOut
    }
    if ($wait.Own)
    {
        Write-Guard ("{0} - inside the hold of {1}" -f $action, $name)
    }
    else
    {
        Write-KeyValueFile $ownerPath @{ name = $name; kind = "run"; action = $action; since = (Format-Time (Get-Date)); pid = $PID }
        Write-Guard ("{0} - lock taken after {1}" -f $action, (Format-Span $wait.Waited))
    }
    if (-not $withApps -and $wait.Activity.Apps.Count -gt 0)
    {
        Write-Guard ("note: also running from Bin\ (shared jg_log.txt): " + ($wait.Activity.Apps -join "; "))
    }
    $started = Get-Date
    $code = 1
    try
    {
        $code = Invoke-Child $file $arguments $workingDirectory
    }
    finally
    {
        $ran = (Get-Date) - $started
        if (-not $wait.Own)
        {
            Exit-TreeLock
        }
        Add-History ("{0} | waited {1} | ran {2} | exit {3}" -f $action, (Format-Span $wait.Waited), (Format-Span $ran), $code)
        Write-Guard ("{0} - exit {1} after {2}" -f $action, $code, (Format-Span $ran))
    }
    return $code
}

# ---- verbs ----

function Find-MSBuild
{
    $candidates = New-Object Collections.Generic.List[string]
    $vswhere = [IO.Path]::Combine(${env:ProgramFiles(x86)}, "Microsoft Visual Studio\Installer\vswhere.exe")
    if ([IO.File]::Exists($vswhere))
    {
        # The engine is built with v143 (Visual Studio 2022); a newer Visual Studio may be installed beside it.
        foreach ($found in (& $vswhere -version "[17.0,18.0)" -products * -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe"))
        {
            $candidates.Add([IO.Path]::Combine([IO.Path]::GetDirectoryName($found), "amd64\MSBuild.exe"))
            $candidates.Add($found)
        }
    }
    $candidates.Add("C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe")
    foreach ($candidate in $candidates)
    {
        if ([IO.File]::Exists($candidate))
        {
            return $candidate
        }
    }
    return $null
}

function Resolve-BuildProject([string]$project)
{
    if ($project -eq "")
    {
        $candidate = $root + "JGEngine.sln"
    }
    elseif ($project.IndexOfAny([char[]]@('\', '/')) -lt 0 -and [IO.Path]::GetExtension($project) -eq "")
    {
        $candidate = $root + "Temp\ProjectFiles\" + $project + ".vcxproj"
    }
    else
    {
        $candidate = [IO.Path]::GetFullPath([IO.Path]::Combine($root, $project.Replace('/', '\')))
    }
    if ([IO.File]::Exists($candidate))
    {
        return $candidate
    }
    return $null
}

function Resolve-Program([string]$program)
{
    $candidate = $program.Replace('/', '\')
    $paths = New-Object Collections.Generic.List[string]
    $paths.Add([IO.Path]::Combine($currentDirectory, $candidate))
    if (-not [IO.Path]::IsPathRooted($candidate) -and -not $candidate.Contains('\'))
    {
        $command = Get-Command $candidate -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($null -ne $command)
        {
            $paths.Add($command.Path)
        }
    }
    foreach ($path in $paths)
    {
        foreach ($suffix in @("", ".exe", ".cmd", ".bat"))
        {
            try
            {
                $full = [IO.Path]::GetFullPath($path + $suffix)
            }
            catch
            {
                continue
            }
            if ([IO.File]::Exists($full))
            {
                return $full
            }
        }
    }
    return $null
}

function Invoke-Status
{
    Write-Guard ("tree " + $root.TrimEnd('\'))
    $busy = $false
    if (Test-TreeLockFree)
    {
        Write-Guard "lock: free"
    }
    else
    {
        Write-Guard (Get-LockHolderText)
        $busy = $true
    }
    foreach ($file in (New-Object IO.DirectoryInfo($guardDirectory)).EnumerateFiles("wait-*.txt"))
    {
        $waiter = Read-KeyValueFile $file.FullName
        if ($null -eq $waiter)
        {
            continue
        }
        if ($null -eq (Get-Process -Id ([int]$waiter["pid"]) -ErrorAction SilentlyContinue))
        {
            try
            {
                $file.Delete()
            }
            catch
            {
            }
            continue
        }
        $since = Parse-Time ([string]$waiter["since"])
        $waited = ""
        if ($null -ne $since)
        {
            $waited = ", for " + (Format-Span ((Get-Date) - $since))
        }
        Write-Guard ("waiting: {0} - {1}{2}" -f $waiter["name"], $waiter["action"], $waited)
    }
    $activity = Get-TreeActivity
    foreach ($item in $activity.Builds)
    {
        Write-Guard ("build: " + $item)
        $busy = $true
    }
    foreach ($item in $activity.Apps)
    {
        Write-Guard ("running Bin\" + $item)
        $busy = $true
    }
    $recent = Get-RecentBuildOutput
    if ($null -ne $recent)
    {
        Write-Guard $recent
        $busy = $true
    }
    foreach ($item in $activity.Ides)
    {
        Write-Guard ("open: " + $item + " (its builds count as builds above)")
    }
    if ($busy)
    {
        Write-Guard "busy"
        exit 1
    }
    Write-Guard "free"
    exit 0
}

function Invoke-Build
{
    $msbuild = Find-MSBuild
    if ($null -eq $msbuild)
    {
        Write-Guard "MSBuild of Visual Studio 2022 not found"
        exit $ExitNotFound
    }
    $project = Resolve-BuildProject $options.project
    if ($null -eq $project)
    {
        Write-Guard ("project not found: " + $options.project)
        exit $ExitNotFound
    }
    $action = "build {0} {1}" -f $options.config, (Get-TreeRelative $project)
    if ($options.target -ne "Build")
    {
        $action += " " + $options.target
    }
    if ($rest -ne "")
    {
        $action += " " + (Get-Shortened $rest 60)
    }
    $arguments = ('"{0}" -t:{1} -p:Configuration={2} -p:Platform=x64 -m -nologo -v:minimal -nr:false {3}' -f $project, $options.target, $options.config, $rest).Trim()
    exit (Invoke-Guarded $action $true $msbuild $arguments $root)
}

function Invoke-Exec
{
    if ($rest -eq "")
    {
        Exit-Usage "exec needs a program"
    }
    $parts = Split-CommandLine $rest
    $program = $parts[0].Value
    $arguments = ""
    if ($parts.Count -gt 1)
    {
        $arguments = $rest.Substring($parts[1].Start)
    }
    $resolved = Resolve-Program $program
    if ($null -eq $resolved)
    {
        Write-Guard ("program not found: " + $program)
        exit $ExitNotFound
    }
    $file = $resolved
    $childArguments = $arguments
    $extension = [IO.Path]::GetExtension($resolved).ToLowerInvariant()
    if ($extension -eq ".cmd" -or $extension -eq ".bat")
    {
        $file = $env:ComSpec
        $childArguments = '/d /s /c ""{0}" {1}"' -f $resolved, $arguments
    }
    elseif ($extension -eq ".ps1")
    {
        $file = $PowerShellPath
        $childArguments = '-NoProfile -ExecutionPolicy Bypass -File "{0}" {1}' -f $resolved, $arguments
    }
    $action = "exec " + (Get-Shortened ([IO.Path]::GetFileName($resolved) + " " + $arguments) 80)
    exit (Invoke-Guarded $action $false $file $childArguments $currentDirectory)
}

function Invoke-Hold
{
    if ($name -eq "unnamed")
    {
        Exit-Usage "hold needs -name=WHO (release and the steps inside the hold use the same name)"
    }
    if ($env:JG_MAINTREE_HELD -eq $root)
    {
        Write-Guard "hold inside another MainTree call - that call already holds the tree"
        exit 0
    }
    if (Test-OwnHold)
    {
        $owner = Read-KeyValueFile $ownerPath
        Write-Guard ("{0} already holds the tree until {1}" -f $name, $owner["until"])
        exit 0
    }
    $token = [Guid]::NewGuid().ToString("N")
    $env:JG_MAINTREE_ARGS = '-root="{0}" -name="{1}" -wait={2} -quiet={3} -minutes={4} -token={5} -parent={6} _holder' -f $root.TrimEnd('\'), $name, $waitMinutes, $quietSeconds, $holdMinutes, $token, $PID
    $holder = Start-Process -FilePath $PowerShellPath -ArgumentList @("-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-WindowStyle", "Hidden", "-File", ('"' + $PSCommandPath + '"')) -WindowStyle Hidden -PassThru
    $null = $holder.Handle
    Remove-Item Env:JG_MAINTREE_ARGS -ErrorAction SilentlyContinue
    $start = Get-Date
    $nextReport = $start.AddSeconds(3)
    while ($true)
    {
        Start-Sleep -Milliseconds 300
        $owner = Read-KeyValueFile $ownerPath
        if ($null -ne $owner -and $owner["token"] -eq $token)
        {
            Write-Guard ("hold taken for {0} until {1} (pid {2}) - build and exec with -name={0} run inside it; end it with: MainTree.cmd -name={0} release" -f $name, $owner["until"].Substring(11), $owner["pid"])
            exit 0
        }
        if ($holder.HasExited)
        {
            Write-Guard ("hold not taken: still busy after {0} minute(s)" -f $waitMinutes)
            exit $ExitTimedOut
        }
        if ((Get-Date) -ge $nextReport)
        {
            $blockers = New-Object Collections.Generic.List[string]
            if (-not (Test-TreeLockFree))
            {
                $blockers.Add((Get-LockHolderText))
            }
            $activity = Get-TreeActivity
            foreach ($item in $activity.Builds)
            {
                $blockers.Add($item)
            }
            foreach ($item in $activity.Apps)
            {
                $blockers.Add("running Bin\" + $item)
            }
            $recent = Get-RecentBuildOutput
            if ($null -ne $recent)
            {
                $blockers.Add($recent)
            }
            Write-Guard ("waiting {0}: {1}" -f (Format-Span ((Get-Date) - $start)), ($blockers -join "; "))
            $nextReport = (Get-Date).AddSeconds(30)
        }
    }
}

# The detached process that keeps a hold: waits like build, then keeps the lock until release or -minutes.
function Invoke-Holder
{
    $token = $options.token
    $parentId = [int]$options.parent
    $action = "hold {0}m" -f $holdMinutes
    $wait = Wait-Tree $action $true $parentId
    if ($null -eq $wait)
    {
        if ($parentId -gt 0 -and $null -eq (Get-Process -Id $parentId -ErrorAction SilentlyContinue))
        {
            Add-History ("{0} | dropped: the hold command ended before the tree was free" -f $action)
        }
        else
        {
            Add-History ("{0} | gave up after {1}m, still busy" -f $action, $waitMinutes)
        }
        exit $ExitTimedOut
    }
    $release = New-Object Threading.EventWaitHandle($false, [Threading.EventResetMode]::ManualReset, ("Local\JGEngineMainTree_" + $token))
    $since = Get-Date
    Write-KeyValueFile $ownerPath @{ name = $name; kind = "hold"; action = $action; since = (Format-Time $since); until = (Format-Time ($since.AddMinutes($holdMinutes))); pid = $PID; token = $token }
    $released = $release.WaitOne([TimeSpan]::FromMinutes($holdMinutes))
    Exit-TreeLock
    $release.Dispose()
    $how = "expired"
    if ($released)
    {
        $how = "released"
    }
    Add-History ("{0} | waited {1} | held {2} | {3}" -f $action, (Format-Span $wait.Waited), (Format-Span ((Get-Date) - $since)), $how)
    exit 0
}

function Invoke-Release
{
    $owner = Read-KeyValueFile $ownerPath
    if ($null -eq $owner -or $owner["kind"] -ne "hold" -or (Test-TreeLockFree))
    {
        Write-Guard "no hold to release"
        exit 1
    }
    if ($owner["name"] -ne $name)
    {
        Write-Guard ("the hold belongs to {0}, not to {1} - left as it is" -f $owner["name"], $name)
        exit 1
    }
    try
    {
        $release = [Threading.EventWaitHandle]::OpenExisting("Local\JGEngineMainTree_" + $owner["token"])
        [void]$release.Set()
        $release.Dispose()
    }
    catch
    {
        Write-Guard ("could not reach the holder (pid {0}): {1}" -f $owner["pid"], $_.Exception.Message)
        exit 1
    }
    for ($attempt = 0; $attempt -lt 50; $attempt++)
    {
        if (Test-TreeLockFree)
        {
            Write-Guard ("hold of {0} released" -f $name)
            exit 0
        }
        Start-Sleep -Milliseconds 200
    }
    Write-Guard "the holder was told to release, but the lock is still taken"
    exit 1
}

# ---- editor: start the editor for the dashboard button ----

# JGLauncher.exe processes started from $launcher: "pid N, HH:mm:ss 부터[, 인자 ...]".
function Get-RunningLaunchers([string]$launcher)
{
    $found = New-Object Collections.Generic.List[string]
    try
    {
        $processes = Get-CimInstance -ClassName Win32_Process -Filter "Name = 'JGLauncher.exe'" -Property ProcessId, ExecutablePath, CommandLine, CreationDate
    }
    catch
    {
        return ,$found
    }
    foreach ($process in $processes)
    {
        if (-not [string]::Equals([string]$process.ExecutablePath, $launcher, [StringComparison]::OrdinalIgnoreCase))
        {
            continue
        }
        $text = "pid " + $process.ProcessId
        if ($null -ne $process.CreationDate)
        {
            $text += ", " + $process.CreationDate.ToString("HH:mm:ss", $Invariant) + " 부터"
        }
        $commandLine = [string]$process.CommandLine
        $parts = Split-CommandLine $commandLine
        if ($parts.Count -gt 1)
        {
            $arguments = $commandLine.Substring($parts[1].Start)
            $text += ", 인자 " + (Get-Shortened $arguments 60)
        }
        $found.Add($text)
    }
    return ,$found
}

function Invoke-Editor
{
    if ($rest -ne "")
    {
        Exit-Usage ("모르는 인자 " + $rest)
    }
    $seconds = 0
    if ($given.ContainsKey("wait"))
    {
        $seconds = $waitMinutes
    }
    $projectMode = $false
    if ($options.project -ne "")
    {
        try
        {
            $projectRoot = [IO.Path]::GetFullPath([IO.Path]::Combine($currentDirectory, $options.project.Replace('/', '\')))
        }
        catch
        {
            Exit-Usage ("-project= 경로가 잘못됨 " + $options.project)
        }
        if ([IO.Path]::GetExtension($projectRoot) -eq ".jgproject")
        {
            $projectRoot = [IO.Path]::GetDirectoryName($projectRoot)
        }
        $projectRoot = $projectRoot.TrimEnd('\') + '\'
        if (-not [string]::Equals($projectRoot, $root, [StringComparison]::OrdinalIgnoreCase))
        {
            if (-not [IO.Directory]::Exists($projectRoot) -or [IO.Directory]::GetFiles($projectRoot, "*.jgproject").Count -eq 0)
            {
                $script:Out.WriteLine("게임 프로젝트가 아님 — " + $projectRoot.TrimEnd('\') + " 에 .jgproject 가 없음")
                exit $ExitNotFound
            }
            # A game project is a tree of its own: only its builds count, and the MainTree lock is not used.
            $projectMode = $true
            $script:root = $projectRoot
            $script:otherTrees = @()
        }
    }
    $binDirectory = $root + "Bin\DevelopEngine"
    $launcher = $binDirectory + "\JGLauncher.exe"
    $label = "Bin\DevelopEngine\JGLauncher.exe"
    if ($projectMode)
    {
        $label = $launcher
    }
    if (-not [IO.File]::Exists($launcher))
    {
        $script:Out.WriteLine("에디터 실행 파일이 없음 — " + $launcher + " (먼저 빌드)")
        exit $ExitNotFound
    }

    # Like Wait-Tree, but in seconds, without the running Bin\ programs and with one line at the end.
    $start = Get-Date
    $deadline = $start.AddSeconds($seconds)
    $locked = $false
    while ($true)
    {
        $blockers = New-Object Collections.Generic.List[string]
        $building = $true
        if (-not $projectMode)
        {
            if (Enter-TreeLock)
            {
                $locked = $true
            }
            else
            {
                $owner = Read-KeyValueFile $ownerPath
                if ($null -eq $owner -or -not $owner.ContainsKey("name"))
                {
                    $blockers.Add("MainTree 잠금(주인 기록 전)")
                }
                else
                {
                    $text = "{0}: {1}" -f $owner["name"], $owner["action"]
                    $since = Parse-Time ([string]$owner["since"])
                    if ($null -ne $since)
                    {
                        $text += ", " + (Format-Span ((Get-Date) - $since)) + "째"
                    }
                    $blockers.Add($text)
                    $building = -not ([string]$owner["action"]).StartsWith("exec")
                }
            }
        }
        if ($blockers.Count -eq 0)
        {
            $activity = Get-TreeActivity
            foreach ($item in $activity.Builds)
            {
                $blockers.Add("도우미 밖 " + $item)
            }
            $recent = Get-RecentBuildOutput
            if ($null -ne $recent)
            {
                $blockers.Add($recent)
            }
        }
        if ($blockers.Count -eq 0)
        {
            break
        }
        if ($locked)
        {
            Close-TreeLock
            $locked = $false
        }
        if ((Get-Date) -ge $deadline)
        {
            $what = "빌드 중"
            if (-not $building)
            {
                $what = "메인 트리 사용 중"
            }
            $waited = ""
            if ($seconds -gt 0)
            {
                $waited = " — {0}초 기다려도 안 끝남" -f $seconds
            }
            Add-History ("editor {0} | busy after {1}s: {2}" -f $label, $seconds, ($blockers -join "; "))
            $script:Out.WriteLine(("{0} — {1}{2}. 끝나면 다시 눌러 주세요" -f $what, ($blockers -join "; "), $waited))
            exit $ExitTimedOut
        }
        Start-Sleep -Milliseconds (1500 + (Get-Random -Maximum 1000))
    }

    # The lock stays taken until the editor runs: a build that starts meanwhile then waits for it like for any
    # editor running from Bin\.
    if ($locked)
    {
        Write-KeyValueFile $ownerPath @{ name = $name; kind = "run"; action = "editor"; since = (Format-Time (Get-Date)); pid = $PID }
    }
    $code = 0
    $running = Get-RunningLaunchers $launcher
    if ($running.Count -gt 0)
    {
        $code = $ExitEditorRunning
        $line = "이미 떠 있음 — {0} {1}. 두 번째는 띄우지 않음" -f $label, ($running -join "; ")
    }
    else
    {
        try
        {
            # Start-Process goes through the shell: the editor gets its own console and none of this process's
            # handles, so a caller reading this output through pipes is not kept waiting by the editor.
            $process = Start-Process -FilePath $launcher -WorkingDirectory $binDirectory -PassThru
            # A launcher that cannot start (a missing or stale DLL) ends within moments.
            if ($process.WaitForExit(2000))
            {
                $code = $ExitEditorFailed
                $line = "에디터가 바로 끝남(종료 코드 {0}) — {1}\jg_log.txt 를 보세요" -f $process.ExitCode, $binDirectory
            }
            else
            {
                $line = "에디터를 띄움(pid {0}) — {1}" -f $process.Id, $label
                if ($seconds -gt 0 -and ((Get-Date) - $start).TotalSeconds -ge 5)
                {
                    $line += ", 빌드가 끝나길 {0} 기다린 뒤" -f (Format-Span ((Get-Date) - $start))
                }
            }
        }
        catch
        {
            $code = $ExitEditorFailed
            $line = "에디터를 띄우지 못함 — " + $_.Exception.Message
        }
    }
    if ($locked)
    {
        Exit-TreeLock
    }
    Add-History ("editor {0} | took {1} | exit {2} | {3}" -f $label, (Format-Span ((Get-Date) - $start)), $code, $line)
    $script:Out.WriteLine($line)
    exit $code
}

switch ($verb)
{
    "status"
    {
        Invoke-Status
    }
    "build"
    {
        Invoke-Build
    }
    "exec"
    {
        Invoke-Exec
    }
    "hold"
    {
        Invoke-Hold
    }
    "release"
    {
        Invoke-Release
    }
    "editor"
    {
        Invoke-Editor
    }
    "_holder"
    {
        Invoke-Holder
    }
    default
    {
        if ($verb -eq "")
        {
            Exit-Usage ""
        }
        Exit-Usage ("unknown command: " + $verb)
    }
}
