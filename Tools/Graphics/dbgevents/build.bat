@echo off
rem Build the debug-event counter (crashwalk stand-in while crashwalk cannot close, Graphics 2026-10-01). usage: build.bat
rem Run: dbgevents.exe <exe> <workdir> <closeAfterSeconds> > out.txt   (engine log + event/exception counts + thread names)
setlocal
set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
call "%VCVARS%" >nul
if errorlevel 1 exit /b 1
cd /d "%~dp0"
cl /nologo /EHsc /O2 /W3 dbgevents.cpp /link /OUT:dbgevents.exe
endlocal
