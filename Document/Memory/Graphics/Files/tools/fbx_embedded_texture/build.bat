@echo off
rem Build the 5-2 test FBX generator. usage: build.bat <out dir>
rem Puts make_embedded_texture_fbx.exe and assimp-vc143-mt.dll in <out dir>. assimp is a release /MD build, so /MD here too.
rem (ASCII only: cmd reads .bat files in the OEM code page)
setlocal
set "HERE=%~dp0"
set "ROOT=%HERE%..\..\..\.."
set "OUT=%~1"
if "%OUT%"=="" set "OUT=%HERE%out"
if not exist "%OUT%" mkdir "%OUT%"

call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1

cl /nologo /EHsc /MD /O2 /std:c++17 /utf-8 /W3 /I "%ROOT%\Source\ThirdParty" "%HERE%make_embedded_texture_fbx.cpp" /Fe"%OUT%\make_embedded_texture_fbx.exe" /Fo"%OUT%\make_embedded_texture_fbx.obj" /link "%ROOT%\ThirdParty\assimp\assimp-mt.lib"
if errorlevel 1 exit /b 1

rem assimp-mt.lib imports assimp-vc143-mt.dll: use the same DLL the engine loads.
copy /Y "%ROOT%\Bin\DevelopEngine\assimp-vc143-mt.dll" "%OUT%\" >nul
if errorlevel 1 exit /b 1

echo build ok: %OUT%\make_embedded_texture_fbx.exe
endlocal
