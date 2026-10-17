@echo off
rem JGEngine: start the editor of this engine tree for the dashboard button (DevOps, 2026-10-17).
rem Starts Bin\DevelopEngine\JGLauncher.exe on its own (working in that folder) and returns at once with one line
rem in Korean (UTF-8 when the output is redirected). The work is done by "MainTree.ps1 editor" next to this file.
rem   RunEditor.cmd [-wait=SECONDS] [-name=WHO] [-project=GAME_PROJECT_DIR]
rem     -wait=SECONDS  wait up to this long (0..600, default 0) for a running build to end, then start
rem     -name=WHO      who asks, for Temp\MainTree\history.log (default RunEditor)
rem     -project=DIR   the editor of a game project instead (DIR\Bin\DevelopEngine); only that project's builds count
rem Exit codes:
rem   0   editor started (the line has its pid)
rem   89  this Bin's editor already runs - no second one
rem   90  busy: the MainTree lock is taken, an MSBuild / cl / link runs outside the helper, or build output was
rem       written in the last 10 seconds - still after -wait (the line names who and what)
rem   91  usage error
rem   92  JGLauncher.exe not built, or -project= has no .jgproject
rem   93  the editor did not start or ended at once (see jg_log.txt of that Bin)
rem The lock is taken only while the editor starts. Builds through MainTree.cmd then wait for the running editor.
setlocal
set "JG_MAINTREE_ARGS=-name=RunEditor editor %*"
set "JG_MAINTREE_PS=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
if exist "%SystemRoot%\Sysnative\WindowsPowerShell\v1.0\powershell.exe" set "JG_MAINTREE_PS=%SystemRoot%\Sysnative\WindowsPowerShell\v1.0\powershell.exe"
"%JG_MAINTREE_PS%" -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "%~dp0MainTree.ps1"
exit /b %ERRORLEVEL%
