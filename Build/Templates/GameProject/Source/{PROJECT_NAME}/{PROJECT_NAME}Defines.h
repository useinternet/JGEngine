#pragma once
#include "Core.h"

// _{PROJECT_NAME_UPPER} 은 {PROJECT_NAME}.module.json 의 Defines 에 있다. 이 모듈을 빌드할 때만 정의된다.
#ifdef _{PROJECT_NAME_UPPER}
#define {PROJECT_NAME_UPPER}_API __declspec(dllexport)
#define {PROJECT_NAME_UPPER}_C_API extern "C" __declspec(dllexport)
#else
#define {PROJECT_NAME_UPPER}_API __declspec(dllimport)
#define {PROJECT_NAME_UPPER}_C_API extern "C" __declspec(dllimport)
#endif
