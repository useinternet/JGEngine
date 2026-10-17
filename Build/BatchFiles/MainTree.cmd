@echo off
rem JGEngine main tree guard (DevOps, 2026-10-03): waits while another session builds or runs this engine tree,
rem then runs the build or the command under a lock. Details and exit codes: MainTree.ps1 next to this file.
rem   MainTree.cmd -name=WHO status
rem   MainTree.cmd -name=WHO [-wait=MINUTES] build [-config=CFG] [-project=NAME_OR_PATH] [-target=T] [MSBuild args]
rem   MainTree.cmd -name=WHO [-wait=MINUTES] exec PROGRAM [args]
rem   MainTree.cmd -name=WHO [-wait=MINUTES] hold [-minutes=N]
rem   MainTree.cmd -name=WHO release
rem   MainTree.cmd -name=WHO [-wait=SECONDS] editor [-project=GAME_PROJECT_DIR]   (RunEditor.cmd)
setlocal
set "JG_MAINTREE_ARGS=%*"
set "JG_MAINTREE_PS=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
if exist "%SystemRoot%\Sysnative\WindowsPowerShell\v1.0\powershell.exe" set "JG_MAINTREE_PS=%SystemRoot%\Sysnative\WindowsPowerShell\v1.0\powershell.exe"
"%JG_MAINTREE_PS%" -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "%~dp0MainTree.ps1"
exit /b %ERRORLEVEL%
