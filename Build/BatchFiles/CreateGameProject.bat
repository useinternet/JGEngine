@echo off
rem Usage: CreateGameProject.bat <ProjectDir> [ProjectName]
rem Creates a game project from Build\Templates\GameProject. ProjectName defaults to the folder name.
rem The name becomes the module, DLL and class name (H<Name>Module), so it must be a C++ identifier.
setlocal
if "%~1"=="" goto usage

set "JG_PROJECT_DIR=%~f1"
if "%JG_PROJECT_DIR:~-1%"=="\" set "JG_PROJECT_DIR=%JG_PROJECT_DIR:~0,-1%"

pushd "%~dp0"
if "%~2"=="" (
	"..\..\Bin\DevelopEngine\JGBuildTool.exe" -newproject="%JG_PROJECT_DIR%"
) else (
	"..\..\Bin\DevelopEngine\JGBuildTool.exe" -newproject="%JG_PROJECT_DIR%" -name="%~2"
)
set "JG_RESULT=%errorlevel%"
popd

if not "%JG_RESULT%"=="0" (
	echo [CreateGameProject] failed. See the log above or Build\BatchFiles\jg_log.txt
	exit /b 1
)

echo [CreateGameProject] created: %JG_PROJECT_DIR%
echo [CreateGameProject] next: "%JG_PROJECT_DIR%\GenerateProjectFiles.bat"
exit /b 0

:usage
echo Usage: CreateGameProject.bat ^<ProjectDir^> [ProjectName]
exit /b 1
