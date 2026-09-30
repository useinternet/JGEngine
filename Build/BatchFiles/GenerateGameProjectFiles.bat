@echo off
rem Usage: GenerateGameProjectFiles.bat <ProjectDir>
rem Generates <ProjectDir>\<Name>.sln (engine projects from this engine + the game modules).
rem Game-module code generation goes to <ProjectDir>\Temp\CodeGen. Engine code generation is read from this
rem engine's Temp\CodeGen, so the engine's GenerateProjectFiles.bat must have run at least once.
setlocal
if "%~1"=="" goto usage

set "JG_PROJECT_DIR=%~f1"
if "%JG_PROJECT_DIR:~-1%"=="\" set "JG_PROJECT_DIR=%JG_PROJECT_DIR:~0,-1%"

if not exist "%JG_PROJECT_DIR%\*.jgproject" (
	echo [GenerateGameProjectFiles] no .jgproject in "%JG_PROJECT_DIR%"
	exit /b 1
)

pushd "%~dp0"
if not exist "..\..\Temp\CodeGen\module_system_info.json" (
	echo [GenerateGameProjectFiles] engine code generation is missing. Run GenerateProjectFiles.bat in the engine root first.
	popd
	exit /b 1
)

"..\..\Bin\DevelopEngine\JGHeaderTool.exe" -project="%JG_PROJECT_DIR%"
if errorlevel 1 goto fail

"..\..\Bin\DevelopEngine\JGBuildTool.exe" -project="%JG_PROJECT_DIR%"
if errorlevel 1 goto fail

popd
echo [GenerateGameProjectFiles] done: %JG_PROJECT_DIR%
exit /b 0

:fail
popd
echo [GenerateGameProjectFiles] failed. See the log above or Build\BatchFiles\jg_log.txt
exit /b 1

:usage
echo Usage: GenerateGameProjectFiles.bat ^<ProjectDir^>
exit /b 1
