@echo off
rem Generates {PROJECT_NAME}.sln with the engine's tools. Run it again after adding or removing source files.
call "{ENGINE_ROOT_WIN}\Build\BatchFiles\GenerateGameProjectFiles.bat" "%~dp0."
if errorlevel 1 pause
