@echo off
rem Usage: BootstrapTools.bat
rem Builds the two build tools, JGHeaderTool.exe and JGBuildTool.exe (DevelopEngine x64), from source into Bin\DevelopEngine.
rem Bin\ is not in git (D-40), so a fresh clone has no tools. PreBuild.bat calls this when either exe is missing.
rem premake5 makes the project files from the tracked jgengine.lua, then the Visual Studio 2022 MSBuild builds Core and
rem the two tools. The PreBuild that follows makes every project file again.
setlocal
pushd "%~dp0..\.."

if not exist ".\premake5.exe" (
	echo [BootstrapTools] premake5.exe is missing in the engine root.
	goto fail
)
if not exist ".\jgengine.lua" (
	echo [BootstrapTools] jgengine.lua is missing in the engine root.
	goto fail
)

set "JG_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "JG_MSBUILD="
if exist "%JG_VSWHERE%" (
	for /f "usebackq delims=" %%i in (`call "%JG_VSWHERE%" -version "[17.0,18.0)" -products * -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\amd64\MSBuild.exe"`) do (
		if not defined JG_MSBUILD set "JG_MSBUILD=%%i"
	)
)
if not defined JG_MSBUILD (
	echo [BootstrapTools] Visual Studio 2022 MSBuild was not found. Install Visual Studio 2022 with "Desktop development with C++".
	goto fail
)

echo [BootstrapTools] Bin\DevelopEngine has no build tools. Building Core, JGHeaderTool and JGBuildTool from source.
.\premake5.exe vs2022 --file=jgengine.lua
if errorlevel 1 goto fail

rem PCH.h links zlibstatic.lib and assimp-mt.lib into every module (#pragma comment(lib)) from Bin\<Config>.
rem JGBuildTool copies them there later, but the tools need them now to link.
if not exist "Bin\DevelopEngine" mkdir "Bin\DevelopEngine"
copy /y "ThirdParty\zlib\zlibstatic.lib" "Bin\DevelopEngine\" >nul
if errorlevel 1 goto copyfail
copy /y "ThirdParty\assimp\assimp-mt.lib" "Bin\DevelopEngine\" >nul
if errorlevel 1 goto copyfail

"%JG_MSBUILD%" "Temp\ProjectFiles\JGHeaderTool.vcxproj" -nologo -v:minimal -m -p:Configuration=DevelopEngine -p:Platform=x64
if errorlevel 1 goto fail
"%JG_MSBUILD%" "Temp\ProjectFiles\JGBuildTool.vcxproj" -nologo -v:minimal -m -p:Configuration=DevelopEngine -p:Platform=x64
if errorlevel 1 goto fail

popd
echo [BootstrapTools] done: Bin\DevelopEngine\JGHeaderTool.exe, Bin\DevelopEngine\JGBuildTool.exe
exit /b 0

:copyfail
echo [BootstrapTools] could not copy zlibstatic.lib and assimp-mt.lib from ThirdParty to Bin\DevelopEngine.

:fail
popd
echo [BootstrapTools] failed. The build tools were not made, so the project files were not generated.
exit /b 1
