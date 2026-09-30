#pragma once
#include "Core.h"

// _{PROJECT_NAME_UPPER}EDITOR 는 {PROJECT_NAME}Editor.module.json 의 Defines 에 있다. 이 모듈을 빌드할 때만 정의된다.
#ifdef _{PROJECT_NAME_UPPER}EDITOR
#define {PROJECT_NAME_UPPER}EDITOR_API __declspec(dllexport)
#define {PROJECT_NAME_UPPER}EDITOR_C_API extern "C" __declspec(dllexport)
#else
#define {PROJECT_NAME_UPPER}EDITOR_API __declspec(dllimport)
#define {PROJECT_NAME_UPPER}EDITOR_C_API extern "C" __declspec(dllimport)
#endif
