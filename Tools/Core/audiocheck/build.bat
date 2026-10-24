@echo off
rem audio_loopback_rms.exe 를 빌드한다. 인자 1 = 엔진 루트(miniaudio.h 위치, 기본 이 폴더 기준 ..\..\..). 출력 = 인자 2 폴더(기본 이 폴더).
setlocal
set "ENGINE_ROOT=%~1"
if "%ENGINE_ROOT%"=="" set "ENGINE_ROOT=%~dp0..\..\.."
set "OUT_DIR=%~2"
if "%OUT_DIR%"=="" set "OUT_DIR=%~dp0"
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%OUT_DIR%"
cl /nologo /EHsc /O2 /W3 /utf-8 /I"%ENGINE_ROOT%\Source\ThirdParty\miniaudio" "%~dp0audio_loopback_rms.cpp" /link /OUT:audio_loopback_rms.exe
