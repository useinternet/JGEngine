@echo off
rem boldlab 빌드(VS 18 x64). 사용: build.bat → boldlab.exe <글꼴.ttf> <출력 접두어> <지금 세기> [여러 배 세기] [윤곽선 세기]
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" > nul
cd /d "%~dp0"
cl /nologo /O2 /EHsc /std:c++17 /utf-8 /W3 /D_CRT_SECURE_NO_WARNINGS boldlab.cpp /Fe:boldlab.exe
