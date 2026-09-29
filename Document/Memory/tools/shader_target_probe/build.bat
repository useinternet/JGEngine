@echo off
rem Build the 5-4 shader target probe. usage: build.bat <out dir>
rem ASCII only: cmd reads .bat files in the OEM code page.
setlocal
set "HERE=%~dp0"
set "OUT=%~1"
if "%OUT%"=="" set "OUT=%HERE%out"
if not exist "%OUT%" mkdir "%OUT%"

call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1

cl /nologo /EHsc /MD /O2 /utf-8 /W3 "%HERE%shader_target_probe.cpp" /Fe"%OUT%\shader_target_probe.exe" /Fo"%OUT%\shader_target_probe.obj"
if errorlevel 1 exit /b 1

echo build ok: %OUT%\shader_target_probe.exe
endlocal
