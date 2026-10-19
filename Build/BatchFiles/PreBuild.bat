

@REM Header Tool -> Build Tool
@REM Bin\ is not in git (D-40). When a tool is missing (fresh clone), BootstrapTools.bat builds both from source first.
@if exist "%~dp0..\..\Bin\DevelopEngine\JGHeaderTool.exe" if exist "%~dp0..\..\Bin\DevelopEngine\JGBuildTool.exe" goto tools
@call "%~dp0BootstrapTools.bat"
@if errorlevel 1 exit /b 1
:tools
call "../../Bin/DevelopEngine/JGHeaderTool.exe"
call "../../Bin/DevelopEngine/JGBuildTool.exe"