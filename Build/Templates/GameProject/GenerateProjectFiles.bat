@echo off
rem Generates {PROJECT_NAME}.sln with the engine's tools. Run it again after adding or removing source files.
call "{ENGINE_ROOT_WIN}\Build\BatchFiles\GenerateGameProjectFiles.bat" "%~dp0."
rem On failure: keep the window open for a double-click run, then return 1 so scripts see the failure
rem (pause alone would end the batch with 0).
if errorlevel 1 (
	pause
	exit /b 1
)
